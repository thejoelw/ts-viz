#pragma once

#include <complex>

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "series/type/finitecompseries.h"

#include "defs/FFT_CACHE_ABOVE_SIZE_LOG2.h"

namespace {

static unsigned long long nextPow2(unsigned long long x) {
    if (x < 2) {
        return 1;
    }
    unsigned int ceilLog2 = (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(x - 1);
    return static_cast<unsigned long long>(1) << ceilLog2;
}

template <template <unsigned int> typename CreateType, unsigned int count>
class DispatchToTemplate;

template <template <unsigned int> typename CreateType>
class DispatchToTemplate<CreateType, 0> {
    template <typename ReturnType, typename... ArgTypes>
    ReturnType call(unsigned int value, ArgTypes &&...) {
        (void) value;
        assert(false);
    }
};

template <template <unsigned int> typename CreateType, unsigned int count>
class DispatchToTemplate {
    template <typename ReturnType, typename... ArgTypes>
    ReturnType call(unsigned int value, ArgTypes &&... args) {
        if (value == count - 1) {
            return CreateType<count - 1>(std::forward<ArgTypes>(args)...);
        } else {
            return DispatchToTemplate<CreateType, count - 1>::template call<ReturnType, ArgTypes...>(value, std::forward<ArgTypes>(args)...);
        }
    }
};

}

namespace series {

// Two types of FFT decompositions:
//   1. Persistent - stored in a time series dependency, and always calculated (whether or not it's needed). Only ever calculated once.
//   2. Transient - calculated when needed, and thrown away.

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    static constexpr std::size_t fftSizeThreshold = 32;

    typedef fftwx_impl<ElementType> fftwx;

public:
    ConvSeries(app::AppContext &context, FiniteCompSeries<ElementType> &kernel, DataSeries<ElementType> &ts, ElementType derivativeOrder, bool backfillZeros)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelBack(kernel.getSize() - 1)
        , sourceSize(kernelBack + CHUNK_SIZE)
        , planSize(nextPow2(sourceSize))
        , derivativeOrder(derivativeOrder)
        , backfillZeros(backfillZeros)
    {
        assert(kernel.getSize() > 0);
    }

    ~ConvSeries() {
        std::unique_lock<std::mutex> lock(fftwMutex);

        for (PlanIO planIO : planIOs) {
            fftwx::free(planIO.in);
            fftwx::free(planIO.fft);
            fftwx::free(planIO.out);
        }

        fftwx::free(kernelFft);

        fftwx::destroy_plan(planBwd);
        fftwx::destroy_plan(planFwd);
    }

    template <unsigned int fillSizeLog2Plus1>
    class Filler {
    public:
        static constexpr unsigned int fillSizeLog2 = fillSizeLog2Plus1 - 1;

        void call(ElementType *dst, unsigned int computedCount) {
            if (fillSizeLog2 >= FFT_CACHE_ABOVE_SIZE_LOG2) {

            } else {

            }
        }
    };

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        std::size_t begin = chunkIndex * CHUNK_SIZE;
        std::size_t end = (chunkIndex + 1) * CHUNK_SIZE;
        if (!backfillZeros && end <= kernelBack) {
            return this->constructChunk([](ElementType *dst, unsigned int computedCount) {
                assert(computedCount == 0);

                std::fill_n(dst, CHUNK_SIZE, NAN);

                return CHUNK_SIZE;
            });
        }

        std::size_t numTsChunks = std::min((sourceSize - 1) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<ChunkPtr<ElementType>> tsChunks;
        tsChunks.reserve(numTsChunks);
        for (std::size_t i = 0; i < numTsChunks; i++) {
            // tsChunks[i] = ts.getChunk(chunkIndex - numTsChunks + i + 1);
            tsChunks.emplace_back(ts.getChunk(chunkIndex - i));
        }

        unsigned int appliedConvs = 0;
        return this->constructChunk([this, appliedConvs, begin, end, tsChunks = std::move(tsChunks)](ElementType *dst, unsigned int computedCount) mutable -> unsigned int {
            for (std::size_t i = 1; i < tsChunks.size(); i++) {
                if (tsChunks[i]->getComputedCount() != CHUNK_SIZE) {
                    return 0;
                }
            }

            unsigned int endCount = tsChunks[0]->getComputedCount();
            if (endCount == computedCount) {
                return endCount;
            }

            while (computedCount < endCount) {
                // Divide kernel into sections: [0:1, 1:2, 2:4, 4:8, 8:16, ...]
                // Result is the convolution of each section with TS, added together
                // [K x T -> R]
                // For dst[0], we add [0:1 x 0:1 -> 0:1]
                // For dst[1], we add [0:1 x 1:2 -> 1:2], [1:2 x 0:1 -> 1:2]
                // For dst[2], we add [0:1 x 2:3 -> 2:3], [1:2 x 1:2 -> 2:3], [2:4 x 0:2 -> 2:4]
                // For dst[3], we add [0:1 x 3:4 -> 3:4], [1:2 x 2:3 -> 3:4]

                // We can jump. We could have processed dst[2:4] as:
                // For dst[2:4], we add [0:2 x 2:4 -> 2:4], [2:4 x 0:2 -> 2:4]

                /*
                10010100 <- prev
                10110110 <- targ
                00000100 <- step

                10011000 <- prev
                10110110 <- targ
                00001000 <- step

                10100000 <- prev
                10110110 <- targ
                00010000 <- step

                10110000 <- prev
                10110110 <- targ
                00000100 <- step

                10110100 <- prev
                10110110 <- targ
                00000010 <- step
                */

                unsigned int countLsz = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(computedCount ^ (computedCount - 1));
                unsigned int maxStep = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(endCount - computedCount);
                unsigned int stepLog2 = std::min(countLsz, maxStep);

                for (unsigned int i = stepLog2; i <= countLsz; i++) {
                    unsigned int kernelBegin = 1u << i;
                    unsigned int kernelEnd = 2u << i;

                    // 2 behaviors for loading fft chunks:
                    // For kernel, maybe just want to load the entire fft spectrum as one thing.
                    // For ts, obviously want to just load what's needed.
                    // Perhaps this means we should calculate the kernel fft spectrum inside ConvSeries?
                }




//                DispatchToTemplate<Filler, CHUNK_SIZE_LOG2>::call(fillSizeLog2Plus1, dst, computedCount);

                computedCount++;
            }

            // Pretty sure that larger FFT sizes only help if we're using them to generate larger blocks.
            // However, with a block size fixed at N, a 2N sized FFT is plenty big.
            // Still have to consider smaller windows, with which a smaller FFT could be beneficial.

            assert(false);


            if (computedCount == 0) {
                // Multiply ffts
            }

            // To compute 0:
            // Add k[0] conv t[0] to [0]

            // To compute 1:
            //

            // I DON'T THINK THIS IS CORRECT:
            // When computing w/2-1:
            // Add k[0:w/2] conv t[0:w/2] to [w/2-1 : w-1]
            // We can either:
            //   (1) handle this off-by-one logic, or <-- DO THIS. NO MORE TS TYPES
            //   (2) use a special kernel type that has everything shifted down by one, and store the zero-element specially.




            // Hack: Can only calculate if all ts chunks are done
            assert(computedCount == 0);
            for (std::size_t i = 0; i < tsChunks.size(); i++) {
                if (tsChunks[i]->getComputedCount() != CHUNK_SIZE) {
                    return 0;
                }
            }

            PlanIO planIO = requestPlanIO();

            static_assert(sizeof(signed int) * CHAR_BIT > CHUNK_SIZE_LOG2, "CHUNK_SIZE_LOG2 is too big");
            static thread_local std::vector<std::pair<signed int, signed int>> nans;
            nans.clear();
            nans.push_back(std::pair<signed int, signed int>(INT_MIN, INT_MIN));

            assert(tsChunks.size() <= planSize / CHUNK_SIZE);
            assert(tsChunks.size() * CHUNK_SIZE <= planSize);
            for (std::size_t i = 0; i < tsChunks.size(); i++) {
                const ChunkPtr<ElementType> &src = tsChunks[i];
                ElementType *dst = planIO.in + planSize - (tsChunks.size() - i) * CHUNK_SIZE;
                for (std::size_t j = 0; j < CHUNK_SIZE; j++) {
                    ElementType val = src->getElement(j);
                    if (std::isnan(val)) {
                        val = 0.0;

                        signed int rangeBegin = j - (tsChunks.size() - i) * CHUNK_SIZE;
                        signed int rangeEnd = rangeBegin + (kernelBack + 1);
                        if (rangeBegin <= nans.back().second) {
                            nans.back().second = rangeEnd;
                        } else {
                            nans.push_back(std::pair<signed int, signed int>(rangeBegin, rangeEnd));
                        }
                    }
                    dst[j] = val;
                }
            }

            std::fill(planIO.in, planIO.in + planSize - std::min(sourceSize, tsChunks.size() * CHUNK_SIZE), static_cast<ElementType>(0.0));

            fftwx::execute_dft_r2c(planFwd, planIO.in, planIO.fft);

            // Elementwise multiply
            for (std::size_t i = 0; i < planSize; i++) {
                fftwx::setComplex(planIO.fft[i], fftwx::getComplex(planIO.fft[i]) * fftwx::getComplex(kernelFft[i]));
            }

            // Backward fft
            fftwx::execute_dft_c2r(planBwd, planIO.fft, planIO.out);

            std::copy_n(planIO.out + (planSize - CHUNK_SIZE), CHUNK_SIZE, dst);

            releasePlanIO(planIO);

            if (!backfillZeros && begin < kernelBack) {
                std::fill_n(dst, std::min(kernelBack, end) - begin, NAN);
            }

            if (!backfillZeros) {
                for (std::pair<signed int, signed int> range : nans) {
                    static constexpr signed int signedChunkSize = CHUNK_SIZE;
                    range.first = std::max(0, signedChunkSize + range.first);
                    range.second = std::min(signedChunkSize, signedChunkSize + range.second);
                    if (range.first < range.second) {
                        std::fill(dst + range.first, dst + range.second, NAN);
                    }
                }
            }

            return CHUNK_SIZE;
        });
    }

private:
    FiniteCompSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    std::size_t kernelBack;
    std::size_t sourceSize;
    std::size_t planSize;
    ElementType derivativeOrder;
    bool backfillZeros;

    bool hasPlan = false;
    typename fftwx::Plan planFwd;
    typename fftwx::Plan planBwd;

    typename fftwx::Complex *kernelFft;

    void processKernel() {
        ElementType *tmpIn = fftwx::alloc_real(planSize);
        typename fftwx::Complex *tmpFft = fftwx::alloc_complex(planSize);

        planFwd = fftwx::plan_dft_r2c_1d(planSize, tmpIn, tmpFft, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        planBwd = fftwx::plan_dft_c2r_1d(planSize, tmpFft, tmpIn, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);

        ElementType invPlanSize = static_cast<ElementType>(1.0) / planSize;
        for (std::size_t i = 0; i < kernel.getSize(); i++) {
            tmpIn[i] = kernel.getData()[i] * invPlanSize;
        }
        std::fill(tmpIn + kernelBack + 1, tmpIn + planSize, static_cast<ElementType>(0.0));
        fftwx::execute(planFwd);

        fftwx::free(tmpIn);

        if (derivativeOrder != 0) {
            for (std::size_t i = 0; i < planSize; i++) {
                std::complex<ElementType> z(0.0, static_cast<ElementType>(i) / planSize);
                std::complex<ElementType> f = std::pow(z, derivativeOrder);
                fftwx::setComplex(tmpFft[i], fftwx::getComplex(tmpFft[i]) * f);
            }
        }

        kernelFft = tmpFft;
    }
};

}

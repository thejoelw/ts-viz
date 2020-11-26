#pragma once

#include <complex>

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "series/type/fftseries.h"

#include "defs/CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_USE_FFT_ABOVE_SIZE_LOG2.h"

namespace {

template <template <unsigned int> typename CreateType, unsigned int count>
class DispatchToTemplate;

template <template <unsigned int> typename ClassType>
class DispatchToTemplate<ClassType, 0> {
    template <typename ReturnType, typename... ArgTypes>
    ReturnType call(unsigned int value, ArgTypes &&...) {
        (void) value;
        assert(false);
    }
};

template <template <unsigned int> typename ClassType, unsigned int count>
class DispatchToTemplate {
    template <typename ReturnType, typename... ArgTypes>
    ReturnType call(unsigned int value, ArgTypes &&... args) {
        if (value == count - 1) {
            return ClassType<count - 1>()(std::forward<ArgTypes>(args)...);
        } else {
            return DispatchToTemplate<ClassType, count - 1>::template call<ReturnType, ArgTypes...>(value, std::forward<ArgTypes>(args)...);
        }
    }
};

}

namespace series {

// Two types of FFT decompositions:
//   1. Persistent - stored in a time series dependency, and always calculated (whether or not it's needed). Only ever calculated once.
//   2. Transient - calculated when needed, and thrown away.


template <typename ElementType>
auto getKernelFftChunks(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t chunkIndex) {
    static constexpr unsigned int begin = CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2;
    static constexpr unsigned int end = CHUNK_SIZE_LOG2 + 1;
    return getKernelFftChunks<ElementType, begin, end>(context, arg, chunkIndex, std::make_index_sequence<end - begin>{});
}
template <typename ElementType, std::size_t begin, std::size_t end, std::size_t... is>
auto getKernelFftChunks(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t chunkIndex, std::index_sequence<is...>) {
#define NUMS (1u << (begin + is))
    return std::tuple<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>...>(FftSeries<ElementType, NUMS>::create(context, arg).getChunk(chunkIndex)...);
#undef NUMS
}

template <typename ElementType>
auto getTsFftChunks(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t offset) {
    static constexpr unsigned int begin = CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2;
    static constexpr unsigned int end = CHUNK_SIZE_LOG2 + 1;
    return getTsFftChunks<ElementType, begin, end>(context, arg, offset, std::make_index_sequence<end - begin>{});
}
template <typename ElementType, std::size_t begin, std::size_t end, std::size_t... is>
auto getTsFftChunks(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t offset, std::index_sequence<is...>) {
#define NUMS (1u << (begin + is))
    return std::tuple<std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>, CHUNK_SIZE / NUMS>...>(getTsFftArr(FftSeries<ElementType, NUMS>::create(context, arg), offset)...);
#undef NUMS
}
template <typename ElementType, std::size_t partitionSize>
auto getTsFftArr(FftSeries<ElementType, partitionSize> &tsFft, std::size_t offset) {
    assert(offset % partitionSize == 0);

    std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, partitionSize * 2>, CHUNK_SIZE / partitionSize> res;
    for (std::size_t i = 0; i < res.size(); i++) {
        res[i] = tsFft.getChunk(offset / partitionSize + i);
    }
    return res;
}

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    typedef fftwx_impl<ElementType> fftwx;
    typedef typename fftwx::Complex ComplexType;

public:
    ConvSeries(app::AppContext &context, DataSeries<ElementType> &kernel, DataSeries<ElementType> &ts, std::size_t kernelSize, bool backfillZeros)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelSize(kernelSize)
        , backfillZeros(backfillZeros)
    {
        assert(kernel.getSize() > 0);

        FftwPlanner<ElementType>::init();
    }

    ~ConvSeries() {
        /*
        std::unique_lock<std::mutex> lock(fftwMutex);

        for (PlanIO planIO : planIOs) {
            fftwx::free(planIO.in);
            fftwx::free(planIO.fft);
            fftwx::free(planIO.out);
        }

        fftwx::free(kernelFft);

        fftwx::destroy_plan(planBwd);
        fftwx::destroy_plan(planFwd);
        */
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        FftSeries<ElementType, CHUNK_SIZE> &kernelFft = FftSeries<ElementType, CHUNK_SIZE>::create(this->context, kernel);
        FftSeries<ElementType, CHUNK_SIZE> &tsFft = FftSeries<ElementType, CHUNK_SIZE>::create(this->context, ts);

        std::size_t numKernelChunks = (kernelSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<ComplexType, CHUNK_SIZE * 2>>> kernelChunks;
        kernelChunks.reserve(numKernelChunks);
        for (std::size_t i = 0; i < numKernelChunks; i++) {
            kernelChunks.emplace_back(kernel.getChunk(i), kernelFft.getChunk(i));
        }

        std::size_t numTsChunks = std::min((kernelSize + CHUNK_SIZE - 2) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<ComplexType, CHUNK_SIZE * 2>>> tsChunks;
        tsChunks.reserve(numTsChunks);
        for (std::size_t i = 0; i < numTsChunks; i++) {
            tsChunks.emplace_back(ts.getChunk(chunkIndex - i), kernelFft.getChunk(chunkIndex - i));
        }

        auto kernelFftChunks0 = getKernelFftChunks<ElementType>(this->context, kernel, 0);
        auto kernelFftChunks1 = getKernelFftChunks<ElementType>(this->context, kernel, 1);

        auto tsFftChunks = getTsFftChunks<ElementType>(this->context, ts, chunkIndex * CHUNK_SIZE);

        return this->constructChunk([
            this,
            kernelChunks = std::move(kernelChunks),
            tsChunks = std::move(tsChunks),
            kernelFftChunks0 = std::move(kernelFftChunks0),
            kernelFftChunks1 = std::move(kernelFftChunks1),
            tsFftChunks = std::move(tsFftChunks)
        ](ElementType *dst, unsigned int computedCount) -> unsigned int {
            for (std::size_t i = 1; i < tsChunks.size(); i++) {
                if (tsChunks[i]->getComputedCount() != CHUNK_SIZE) {
                    return 0;
                }
            }

            unsigned int endCount = tsChunks[0]->getComputedCount();
            if (endCount == computedCount) {
                return endCount;
            }

            if (computedCount == 0) {
                typename fftwx::Plan planFwd = FftwPlanner<ElementType>::template getPlanFwd<fillSize>();
                typename fftwx::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<fillSize>();

                const typename FftwPlanner<ElementType>::IO kernelPlanIO = fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2
                        ? FftwPlanner<ElementType>::request() : FftwPlanner<ElementType>::IO();
                const typename FftwPlanner<ElementType>::IO tsPlanIO = fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2
                        ? FftwPlanner<ElementType>::request() : FftwPlanner<ElementType>::IO();
                const typename FftwPlanner<ElementType>::IO eitherPlanIO =
                        fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 ? kernelPlanIO : fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 ? tsPlanIO : FftwPlanner<ElementType>::request();

                typename fftwx::Complex *kernelFft;
                if (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    kernelFft = std::get<ChunkPtr<ElementType, 2u << fillSizeLog2>>(kernelFftChunks)->getVolatileData();
                } else {
                    std::copy_n(kernelChunk->getVolatileData() + kernelOffset, fillSize, kernelPlanIO.in);
                    std::fill_n(kernelPlanIO.in, fillSize, static_cast<ElementType>(0.0));
                    fftwx::execute_dft_r2c(planFwd, kernelPlanIO.in, kernelPlanIO.fft);
                    kernelFft = kernelPlanIO.fft;
                }

                typename fftwx::Complex *tsFft;
                if (fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {

                } else {
                    std::copy_n(tsChunk->getVolatileData() + tsOffset, fillSize, tsPlanIO.in);
                    std::fill_n(tsPlanIO.in, fillSize, static_cast<ElementType>(0.0));
                    fftwx::execute_dft_r2c(planFwd, tsPlanIO.in, tsPlanIO.fft);
                    tsFft = tsPlanIO.fft;
                }

                // Elementwise multiply
                for (std::size_t i = 0; i < fillSize; i++) {
                    fftwx::setComplex(eitherPlanIO.fft[i], fftwx::getComplex(kernelFft[i]) * fftwx::getComplex(tsFft[i]));
                }

                // Backward fft
                FftwPlanner<ElementType>::checkAlignment(kernelPlanIO.fft);
                FftwPlanner<ElementType>::checkAlignment(dst + computedCount);
                fftwx::execute_dft_c2r(planBwd, kernelPlanIO.fft, dst + computedCount);

                // Release stuff
                if (fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(kernelPlanIO);
                }
                if (fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(tsPlanIO);
                }
                if (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 && fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(eitherPlanIO);
                }
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
                    DispatchToTemplate<KernelPartitionFiller, CHUNK_SIZE_LOG2 + 1>::call(i, dst, computedCount, kernelFftChunks1);

                    unsigned int kernelBegin = 1u << i;
                    unsigned int kernelEnd = 2u << i;

                    // 2 behaviors for loading fft chunks:
                    // For kernel, maybe just want to load the entire fft spectrum as one thing.
                    // For ts, obviously want to just load what's needed.
                    // Perhaps this means we should calculate the kernel fft spectrum inside ConvSeries?
                }

                DispatchToTemplate<KernelPartitionFiller, CHUNK_SIZE_LOG2 + 1>::call(stepLog2, dst, computedCount, kernelFftChunks0);

                computedCount += 1u << stepLog2;
            }

            // Pretty sure that larger FFT sizes only help if we're using them to generate larger blocks.
            // However, with a block size fixed at N, a 2N sized FFT is plenty big.
            // Still have to consider smaller windows, with which a smaller FFT could be beneficial.

            assert(false);
            /*


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
            */

            return CHUNK_SIZE;
        });
    }

    template <unsigned int fillSizeLog2>
    struct KernelPartitionFiller {
        void operator()(
                ElementType *dst,
                unsigned int computedCount,
                const ChunkPtr<ElementType> &kernelChunk,
                const ChunkPtr<ElementType> &tsChunk,
                const typename std::result_of<decltype(&getKernelFftChunks<ElementType>)>::type &kernelFftChunks,
                bool kernelPartitionIndex // Either the first or second partition
        ) {
            static_assert(fillSizeLog2 <= CHUNK_SIZE_LOG2, "We should not be generating code to handle larger-than-chunk-size fills");

            static_assert((sizeof(ElementType) << CONV_USE_FFT_ABOVE_SIZE_LOG2) >= 16, "Data subscripts won't be aligned to 16-byte boundaries");

            static constexpr unsigned int fillSize = 1u << fillSizeLog2;
            assert(computedCount % fillSize == 0);

            unsigned int kernelOffset = fillSize * kernelPartitionIndex;
            unsigned int tsOffset = computedCount - fillSize * kernelPartitionIndex;

            if (fillSizeLog2 >= CONV_USE_FFT_ABOVE_SIZE_LOG2) {
                typename fftwx::Plan planFwd = FftwPlanner<ElementType>::template getPlanFwd<fillSize>();
                typename fftwx::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<fillSize>();

                const typename FftwPlanner<ElementType>::IO kernelPlanIO = fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2
                        ? FftwPlanner<ElementType>::request() : FftwPlanner<ElementType>::IO();
                const typename FftwPlanner<ElementType>::IO tsPlanIO = fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2
                        ? FftwPlanner<ElementType>::request() : FftwPlanner<ElementType>::IO();
                const typename FftwPlanner<ElementType>::IO eitherPlanIO =
                        fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 ? kernelPlanIO : fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 ? tsPlanIO : FftwPlanner<ElementType>::request();

                typename fftwx::Complex *kernelFft;
                if (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    kernelFft = std::get<ChunkPtr<ElementType, 2u << fillSizeLog2>>(kernelFftChunks)->getVolatileData();
                } else {
                    std::copy_n(kernelChunk->getVolatileData() + kernelOffset, fillSize, kernelPlanIO.in);
                    std::fill_n(kernelPlanIO.in, fillSize, static_cast<ElementType>(0.0));
                    fftwx::execute_dft_r2c(planFwd, kernelPlanIO.in, kernelPlanIO.fft);
                    kernelFft = kernelPlanIO.fft;
                }

                typename fftwx::Complex *tsFft;
                if (fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {

                } else {
                    std::copy_n(tsChunk->getVolatileData() + tsOffset, fillSize, tsPlanIO.in);
                    std::fill_n(tsPlanIO.in, fillSize, static_cast<ElementType>(0.0));
                    fftwx::execute_dft_r2c(planFwd, tsPlanIO.in, tsPlanIO.fft);
                    tsFft = tsPlanIO.fft;
                }

                // Elementwise multiply
                for (std::size_t i = 0; i < fillSize; i++) {
                    fftwx::setComplex(eitherPlanIO.fft[i], fftwx::getComplex(kernelFft[i]) * fftwx::getComplex(tsFft[i]));
                }

                // Backward fft
                FftwPlanner<ElementType>::checkAlignment(kernelPlanIO.fft);
                FftwPlanner<ElementType>::checkAlignment(dst + computedCount);
                fftwx::execute_dft_c2r(planBwd, kernelPlanIO.fft, dst + computedCount);

                // Release stuff
                if (fillSizeLog2 < CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(kernelPlanIO);
                }
                if (fillSizeLog2 < CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(tsPlanIO);
                }
                if (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 && fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                    FftwPlanner<ElementType>::release(eitherPlanIO);
                }
            } else {
                assert(fillSizeLog2 < CHUNK_SIZE_LOG2);
                assert(computedCount + 1 >= fillSize);

                for (unsigned int i = 0; i < fillSize; i++) {
                    for (unsigned int j = 0; j < fillSize; j++) {
                        dst[computedCount] += kernelChunk->getElement(kernelOffset + j) * tsChunk->getElement(tsOffset + j);
                    }
                    computedCount++;
                }
            }
        }
    };

private:
    DataSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    std::size_t kernelSize;
    bool backfillZeros;

    /*
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
    */
};

}

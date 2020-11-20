#pragma once

#include <complex>

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "series/type/finitecompseries.h"

namespace {

static unsigned long long nextPow2(unsigned long long x) {
    if (x < 2) {
        return 1;
    }
    unsigned int ceilLog2 = (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(x - 1);
    return static_cast<unsigned long long>(1) << ceilLog2;
}

}

namespace series {

extern std::mutex fftwMutex;

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

        return this->constructChunk([this, begin, end, tsChunks = std::move(tsChunks)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            for (std::size_t i = 1; i < tsChunks.size(); i++) {
                if (tsChunks[i]->getComputedCount() != CHUNK_SIZE) {
                    return 0;
                }
            }

            unsigned int count = tsChunks[0]->getComputedCount();
            if (count == computedCount) {
                return count;
            }

            // Pretty sure that larger FFT sizes only help if we're using them to generate larger blocks.
            // However, with a block size fixed at N, a 2N sized FFT is plenty big.
            // Still have to consider smaller windows, with which a smaller FFT could be beneficial.

            assert(false);

            if (computedCount == 0) {
                // Multiply ffts

            }



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

            std::copy_n(planIO.out + planSize - CHUNK_SIZE, CHUNK_SIZE, dst);

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

    struct PlanIO {
        ElementType *in;
        typename fftwx::Complex *fft;
        ElementType *out;
    };
    std::vector<PlanIO> planIOs;

    PlanIO requestPlanIO() {
        std::unique_lock<std::mutex> lock(fftwMutex);
        if (!hasPlan) {
            processKernel();
            hasPlan = true;
        }

        return makePlanIO();
    }

    PlanIO makePlanIO() {
        if (planIOs.empty()) {
            PlanIO res;
            res.in = fftwx::alloc_real(planSize);
            res.fft = fftwx::alloc_complex(planSize);
            res.out = fftwx::alloc_real(planSize);
            return res;
        } else {
            PlanIO res = planIOs.back();
            planIOs.pop_back();
            return res;
        }
    }

    void releasePlanIO(PlanIO planIO) {
        std::unique_lock<std::mutex> lock(fftwMutex);
        planIOs.push_back(planIO);
    }

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

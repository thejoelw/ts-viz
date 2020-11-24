#pragma once

#include <complex>
#include <unordered_map>

#include "series/dataseries.h"
#include "series/fftwx.h"

namespace series {

// Two types of FFT decompositions:
//   1. Persistent - stored in a time series dependency, and always calculated (whether or not it's needed). Only ever calculated once.
//   2. Transient - calculated when needed, and thrown away.

template <typename ElementType, std::size_t partitionSize>
class FftSeries : public DataSeries<typename fftwx_impl<ElementType>::Complex, partitionSize * 2> {
    static_assert(partitionSize <= CHUNK_SIZE, "FftSeries doesn't support calculating ffts on multiple chunks");

    // With zero-padding, the fft is twice as big as the input
    static constexpr std::size_t fftSize = partitionSize * 2;

private:
    typedef fftwx_impl<ElementType> fftwx;

public:
    static FftSeries<ElementType, partitionSize> &create(app::AppContext &context, DataSeries<ElementType> &arg) {
        static std::unordered_map<DataSeries<ElementType> *, FftSeries<ElementType, partitionSize> *> cache;
        auto foundValue = cache.emplace(&arg, 0);
        if (foundValue.second) {
            foundValue.first->second = new FftSeries<ElementType, partitionSize>(context, arg);
        }
        return *foundValue.first->second;
    }

private:
    FftSeries(app::AppContext &context, DataSeries<ElementType> &arg)
        : DataSeries<ElementType>(context)
        , arg(arg)
    {}

public:
    ~FftSeries() {
        std::unique_lock<std::mutex> lock(fftwMutex);
        fftwx::destroy_plan(planFwd);
    }

    Chunk<ElementType, fftSize> *makeChunk(std::size_t chunkIndex) override {
        ChunkPtr<ElementType> chunk = arg.getChunk(chunkIndex * partitionSize / CHUNK_SIZE);
        unsigned int offset = chunkIndex * partitionSize % CHUNK_SIZE;

        return this->constructChunk([this, chunk = std::move(chunk), offset](ElementType *dst, unsigned int computedCount) -> unsigned int {
            assert(computedCount == 0);

            // This is an all-or-nothing transform
            if (chunk->getComputedCount() < offset + partitionSize) {
                return 0;
            }

            FftwPlanIO<ElementType> planIO = FftwPlanIO<ElementType>::request([this](FftwPlanIO<ElementType> planIO) {
                if (!hasPlan) {
                    planFwd = fftwx::plan_dft_r2c_1d(fftSize, planIO.in, planIO.fft, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
                    hasPlan = true;
                }
            });

            for (std::size_t i = 0; i < partitionSize; i++) {
                ElementType val = chunk->getElement(offset + i);
                if (std::isnan(val)) {
                    val = 0.0;

//                    signed int rangeBegin = j - (tsChunks.size() - i) * CHUNK_SIZE;
//                    signed int rangeEnd = rangeBegin + (kernelBack + 1);
//                    if (rangeBegin <= nans.back().second) {
//                        nans.back().second = rangeEnd;
//                    } else {
//                        nans.push_back(std::pair<signed int, signed int>(rangeBegin, rangeEnd));
//                    }
                }
                planIO.in[i] = val;
            }

            std::fill_n(planIO.in + partitionSize, partitionSize, static_cast<ElementType>(0.0));

            fftwx::execute_dft_r2c(planFwd, planIO.in, planIO.fft);

            std::copy_n(planIO.fft, fftSize, dst);

            FftwPlanIO<ElementType>::release(planIO);

            return fftSize;
        });
    }

private:
    DataSeries<ElementType> &arg;

    bool hasPlan = false;
    typename fftwx::Plan planFwd;
};

}

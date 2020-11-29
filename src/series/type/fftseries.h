#pragma once

#include <complex>
#include <unordered_map>

#include "series/dataseries.h"
#include "series/fftwx.h"

namespace series {

// Two types of FFT decompositions:
//   1. Persistent - stored in a time series dependency, and always calculated (whether or not it's needed). Only ever calculated once.
//   2. Transient - calculated when needed, and thrown away.

enum class PaddingType {
    ZeroFill,
    PriorChunk,
};

template <typename ElementType, std::size_t partitionSize, PaddingType paddingType, typename scale = std::ratio<1>>
class FftSeries : public DataSeries<typename fftwx_impl<ElementType>::Complex, partitionSize * 2> {
    static_assert(partitionSize <= CHUNK_SIZE, "FftSeries doesn't support calculating ffts on multiple chunks");

    // With zero-padding, the fft is twice as big as the input
    static constexpr std::size_t fftSize = partitionSize * 2;

    static constexpr ElementType factor = static_cast<ElementType>(scale::num) / static_cast<ElementType>(scale::den);

private:
    typedef fftwx_impl<ElementType> fftwx;
    typedef typename fftwx::Complex ComplexType;

public:
    static FftSeries<ElementType, partitionSize, paddingType, scale> &create(app::AppContext &context, DataSeries<ElementType> &arg) {
        static std::unordered_map<DataSeries<ElementType> *, FftSeries<ElementType, partitionSize, paddingType, scale> *> cache;
        auto foundValue = cache.emplace(&arg, static_cast<FftSeries<ElementType, partitionSize, paddingType, scale> *>(0));
        if (foundValue.second) {
            foundValue.first->second = new FftSeries<ElementType, partitionSize, paddingType, scale>(context, arg);
        }
        return *foundValue.first->second;
    }

private:
    FftSeries(app::AppContext &context, DataSeries<ElementType> &arg)
        : DataSeries<typename fftwx_impl<ElementType>::Complex, partitionSize * 2>(context)
        , arg(arg)
    {
        FftwPlanner<ElementType>::init();
    }

public:
    Chunk<ComplexType, fftSize> *makeChunk(std::size_t chunkIndex) override {
        ChunkPtr<ElementType> prevChunk = chunkIndex > 0 && paddingType == PaddingType::PriorChunk
                ? arg.getChunk((chunkIndex - 1) * partitionSize / CHUNK_SIZE)
                : ChunkPtr<ElementType>::null();
        ChunkPtr<ElementType> curChunk = arg.getChunk(chunkIndex * partitionSize / CHUNK_SIZE);

        return this->constructChunk([chunkIndex, prevChunk = std::move(prevChunk), curChunk = std::move(curChunk)](ComplexType *dst, unsigned int computedCount) -> unsigned int {
            unsigned int prevOffset = (chunkIndex - 1) * partitionSize % CHUNK_SIZE;
            unsigned int curOffset = chunkIndex * partitionSize % CHUNK_SIZE;

            assert(computedCount == 0);

            // This is an all-or-nothing transform
            if (paddingType == PaddingType::PriorChunk && prevChunk.has() && prevChunk->getComputedCount() < prevOffset + partitionSize) {
                return 0;
            }
            if (curChunk->getComputedCount() < curOffset + partitionSize) {
                return 0;
            }

            typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();
            doFft(dst, prevChunk, prevOffset, curChunk, curOffset, planIO.in);
            FftwPlanner<ElementType>::release(planIO);

            return fftSize;
        });
    }

    static void doFft(ComplexType *dst, const ChunkPtr<ElementType> &prevChunk, unsigned int prevOffset, const ChunkPtr<ElementType> &curChunk, unsigned int curOffset, ElementType *scratch) {
        typename fftwx::Plan plan = FftwPlanner<ElementType>::template getPlanFwd<fftSize>();

        if (paddingType == PaddingType::PriorChunk && prevChunk.has()) {
            for (std::size_t i = 0; i < partitionSize; i++) {
                ElementType val = prevChunk->getElement(prevOffset + i);
                scratch[i] = std::isnan(val) ? 0.0 : val * factor;
            }
        } else {
            std::fill_n(scratch, partitionSize, static_cast<ElementType>(0.0));
        }

        for (std::size_t i = 0; i < partitionSize; i++) {
            ElementType val = curChunk->getElement(curOffset + i);
            scratch[partitionSize + i] = std::isnan(val) ? 0.0 : val * factor;
        }

        FftwPlanner<ElementType>::checkAlignment(dst);
        fftwx::execute_dft_r2c(plan, scratch, dst);
    }

private:
    DataSeries<ElementType> &arg;
};

}

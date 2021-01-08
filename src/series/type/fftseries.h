#pragma once

#include <complex>
#include <unordered_map>

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "util/constexprcontrol.h"

namespace {

template <typename T>
static constexpr T divFloor(T a, T b) {
    assert(b > 0);
    return a >= 0 ? a / b : -((b - a - 1) / b);
}
template <typename T>
static constexpr T remFloor(T a, T b) {
    assert(b > 0);
    return a >= 0 ? a % b : b - (b - a - 1) % b - 1;
}

static_assert(divFloor(-5, 4) == -2, "divFloor test #1 failure");
static_assert(divFloor(-4, 4) == -1, "divFloor test #2 failure");
static_assert(divFloor(-3, 4) == -1, "divFloor test #3 failure");
static_assert(divFloor(-2, 4) == -1, "divFloor test #4 failure");
static_assert(divFloor(-1, 4) == -1, "divFloor test #5 failure");
static_assert(divFloor(0, 4) == 0, "divFloor test #6 failure");
static_assert(divFloor(1, 4) == 0, "divFloor test #7 failure");
static_assert(divFloor(2, 4) == 0, "divFloor test #8 failure");
static_assert(divFloor(3, 4) == 0, "divFloor test #9 failure");
static_assert(divFloor(4, 4) == 1, "divFloor test #10 failure");
static_assert(divFloor(5, 4) == 1, "divFloor test #11 failure");

static_assert(remFloor(-5, 4) == 3, "divFloor test #1 failure");
static_assert(remFloor(-4, 4) == 0, "divFloor test #2 failure");
static_assert(remFloor(-3, 4) == 1, "divFloor test #3 failure");
static_assert(remFloor(-2, 4) == 2, "divFloor test #4 failure");
static_assert(remFloor(-1, 4) == 3, "divFloor test #5 failure");
static_assert(remFloor(0, 4) == 0, "divFloor test #6 failure");
static_assert(remFloor(1, 4) == 1, "divFloor test #7 failure");
static_assert(remFloor(2, 4) == 2, "divFloor test #8 failure");
static_assert(remFloor(3, 4) == 3, "divFloor test #9 failure");
static_assert(remFloor(4, 4) == 0, "divFloor test #10 failure");
static_assert(remFloor(5, 4) == 1, "divFloor test #11 failure");

template <std::size_t partitionSize, signed int srcOffset, unsigned int copySize>
struct MaxNumChunksCalculator {
private:
    static constexpr unsigned int partitionSizeLsb = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(partitionSize ^ (partitionSize - 1));
    static constexpr std::size_t minPartitionOffset = 1u << std::min<unsigned int>(partitionSizeLsb, CHUNK_SIZE_LOG2);
    static constexpr signed int latestPartitionBegin = remFloor<signed int>(srcOffset, minPartitionOffset) - minPartitionOffset;
    static_assert(latestPartitionBegin < 0, "Partition doesn't begin in previous chunk");
    static constexpr std::size_t maxNumChunks = (latestPartitionBegin + copySize + CHUNK_SIZE * 2 - 1) / CHUNK_SIZE;
    static_assert(maxNumChunks <= (copySize - 2) / CHUNK_SIZE + 2, "maxNunChunks is larger than a quick calculation on the maximum number of chunks a copySize range can span");
    static_assert(maxNumChunks > 0, "Weird");

public:
    static constexpr std::size_t value = maxNumChunks;
};

static constexpr std::size_t CS = CHUNK_SIZE;
static_assert(MaxNumChunksCalculator<8, -8, 8>::value == 1, "MaxNumChunksCalculator test #1 failure");
static_assert(MaxNumChunksCalculator<8, -8, 9>::value == 2, "MaxNumChunksCalculator test #2 failure");
static_assert(MaxNumChunksCalculator<8, -7, 8>::value == 2, "MaxNumChunksCalculator test #3 failure");
static_assert(MaxNumChunksCalculator<8, -7, 9>::value == 2, "MaxNumChunksCalculator test #4 failure");
static_assert(MaxNumChunksCalculator<8, -1, 8>::value == 2, "MaxNumChunksCalculator test #5 failure");
static_assert(MaxNumChunksCalculator<8, -1, 9>::value == 2, "MaxNumChunksCalculator test #6 failure");
static_assert(MaxNumChunksCalculator<8, 0, 8>::value == 1, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<8, 0, 9>::value == 2, "MaxNumChunksCalculator test #8 failure");
static_assert(MaxNumChunksCalculator<CS, 0, CS>::value == 1, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS, 0, CS + 1>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS, 1, CS>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS + 1, 0, CS>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS + 1, 0, CS + 1>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS + 1, 0, CS + 2>::value == 3, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS - 1, 0, CS>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS - 1, 0, CS + 1>::value == 2, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS - 1, 0, CS + 2>::value == 3, "MaxNumChunksCalculator test #7 failure");
static_assert(MaxNumChunksCalculator<CS * 2, 0, CS>::value == 1, "MaxNumChunksCalculator test #8 failure");
static_assert(MaxNumChunksCalculator<CS * 2, -1, CS>::value == 2, "MaxNumChunksCalculator test #8 failure");
static_assert(MaxNumChunksCalculator<CS * 2, 1, CS>::value == 2, "MaxNumChunksCalculator test #8 failure");
static_assert(MaxNumChunksCalculator<CS * 2, 0, CS * 2>::value == 2, "MaxNumChunksCalculator test #8 failure");

}

namespace series {

// Two types of FFT decompositions:
//   1. Persistent - stored in a time series dependency, and always calculated (whether or not it's needed). Only ever calculated once.
//   2. Transient - calculated when needed, and thrown away.

template <typename ElementType, std::size_t partitionSize, signed int srcOffset, unsigned int copySize, unsigned int dstOffset, typename scale = std::ratio<1>>
class FftSeries : public DataSeries<typename fftwx_impl<ElementType>::Complex, partitionSize * 2> {
    typedef FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset, scale> SelfType;

private:
    static_assert(partitionSize > 0, "Partition size must be positive");
    static_assert((partitionSize & (partitionSize - 1)) == 0, "Partition size must be a power of 2");
    static_assert(partitionSize <= CHUNK_SIZE, "Partition size is too big");
    static_assert(srcOffset >= -partitionSize, "Src offset is too far back (could span multiple chunks)");
    static_assert(srcOffset + copySize <= partitionSize, "Src offset + copy size is too far forwards (could span multiple chunks)");
    static_assert(copySize > 0, "Must copy a positive size of data");

    // The fft is twice as big as the input
    static constexpr std::size_t fftSize = partitionSize * 2;
    static_assert(dstOffset + copySize <= fftSize, "The copied elements would exceed the size of the FFT");

    static constexpr ElementType factor = static_cast<ElementType>(scale::num) / static_cast<ElementType>(scale::den);

    typedef fftwx_impl<ElementType> fftwx;
    typedef typename fftwx::Complex ComplexType;

public:
    static SelfType &create(app::AppContext &context, DataSeries<ElementType> &arg) {
        static std::unordered_map<DataSeries<ElementType> *, SelfType *> cache;
        auto foundValue = cache.emplace(&arg, static_cast<SelfType *>(0));
        if (foundValue.second) {
            foundValue.first->second = new SelfType(context, arg);
#if ENABLE_CHUNK_NAMES
            foundValue.first->second->setName("fft<" + std::to_string(partitionSize) + ">");
#endif
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
    static constexpr bool hasPrevChunk = srcOffset < 0;
    static constexpr bool hasCurChunk = srcOffset + copySize > 0;
    static_assert(hasPrevChunk || hasCurChunk, "Not using any data chunks");

    Chunk<ComplexType, fftSize> *makeChunk(std::size_t chunkIndex) override {
        std::size_t centerOffset = chunkIndex * partitionSize;

        ChunkPtr<ElementType> prevChunk = hasPrevChunk && static_cast<signed int>(centerOffset) >= -srcOffset
                ? arg.getChunk(static_cast<std::size_t>(centerOffset + srcOffset) / CHUNK_SIZE)
                : ChunkPtr<ElementType>::null();
        ChunkPtr<ElementType> curChunk = hasCurChunk
                ? arg.getChunk(centerOffset / CHUNK_SIZE)
                : ChunkPtr<ElementType>::null();

        return this->constructChunk([centerOffset, prevChunk = std::move(prevChunk), curChunk = std::move(curChunk)](ComplexType *dst, unsigned int computedCount) -> unsigned int {
            assert(computedCount == 0);

            // This is an all-or-nothing transform
            assert(hasCurChunk == curChunk.has());
            if (hasCurChunk && prevChunk->getComputedCount() < static_cast<std::size_t>(centerOffset + (srcOffset + copySize)) % CHUNK_SIZE) {
                return 0;
            }
            assert(hasPrevChunk || !prevChunk.has());
            if (hasPrevChunk && prevChunk.has() && prevChunk->getComputedCount() < static_cast<std::size_t>(centerOffset + srcOffset) % CHUNK_SIZE) {
                return 0;
            }

            typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();
            doFft(dst, prevChunk, curChunk, centerOffset, planIO.real);
            FftwPlanner<ElementType>::release(planIO);

            return fftSize;
        });
    }

    static void doFft(ComplexType *dst, const ChunkPtr<ElementType> &prevChunk, const ChunkPtr<ElementType> &curChunk, unsigned int centerOffset, ElementType *scratch) {
#ifndef NDEBUG
        std::fill_n(scratch, fftSize, ElementType(NAN));
#endif

        static constexpr unsigned int dstSplit = dstOffset - std::min(0, srcOffset);

        assert(hasPrevChunk || !prevChunk.has());
        if (hasPrevChunk && prevChunk.has()) {
            static constexpr signed int size = std::min<signed int>(-srcOffset, copySize);
            static_assert(size > 0, "Previous chunk's size is not positive");
            static_assert(size <= partitionSize, "Previous chunk's size is too big");
            zeroRange<0, dstOffset>(scratch);

            unsigned int srcChunkOffset = (centerOffset + srcOffset) % CHUNK_SIZE;
            for (std::size_t i = 0; i < size; i++) {
                ElementType val = prevChunk->getElement(srcChunkOffset + i);
                scratch[dstOffset + i] = std::isnan(val) ? 0.0 : val * factor;
            }

            zeroRange<dstOffset + size, dstSplit>(scratch);
        } else {
            zeroRange<0, dstSplit>(scratch);
        }

        assert(hasCurChunk == curChunk.has());
        if (hasCurChunk) {
            static constexpr signed int size = std::min<signed int>(copySize + srcOffset, copySize);
            static_assert(size > 0, "Current chunk's size is not positive");
            static_assert(size <= partitionSize, "Current chunk's size is too big");

            unsigned int srcChunkOffset = centerOffset % CHUNK_SIZE;
            for (std::size_t i = 0; i < size; i++) {
                ElementType val = curChunk->getElement(srcChunkOffset + i);
                scratch[dstSplit + i] = std::isnan(val) ? 0.0 : val * factor;
            }

            zeroRange<dstSplit + size, fftSize>(scratch);
        } else {
            zeroRange<dstSplit, fftSize>(scratch);
        }

#ifndef NDEBUG
        for (std::size_t i = 0; i < fftSize; i++) {
            assert(!std::isnan(scratch[i]));
        }
#endif

        typename fftwx::Plan plan = FftwPlanner<ElementType>::template getPlanFwd<fftSize>();
        FftwPlanner<ElementType>::checkAlignment(dst);
        fftwx::execute_dft_r2c(plan, scratch, dst);
    }

private:
    DataSeries<ElementType> &arg;

    template <std::size_t begin, std::size_t end>
    static void zeroRange(ElementType *dst) {
        static_assert(begin <= end, "Range has negative size!");
        std::fill(dst + begin, dst + end, ElementType(0.0));
    }
};

}

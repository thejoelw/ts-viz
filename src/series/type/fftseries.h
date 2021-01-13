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
    static constexpr signed int splitOffset = (divFloor<signed int>(srcOffset, partitionSize) + 1) * partitionSize;
    static constexpr signed int beginOffsetFromSplit = srcOffset - splitOffset;
    static_assert(beginOffsetFromSplit >= -static_cast<signed int>(partitionSize), "Begins more than one partition back");
    static_assert(beginOffsetFromSplit < 0, "Does not have a left chunk");
    static constexpr signed int endOffsetFromSplit = srcOffset + copySize - splitOffset;
    static_assert(endOffsetFromSplit <= static_cast<signed int>(partitionSize), "Ends more than one partition forward");
    static constexpr bool hasRightChunk = endOffsetFromSplit > 0;

    typedef std::make_signed<std::size_t>::type signed_size_t;

    Chunk<ComplexType, fftSize> *makeChunk(std::size_t chunkIndex) override {
        signed_size_t centerOffset = chunkIndex * partitionSize + splitOffset;

        ChunkPtr<ElementType> leftChunk = centerOffset >= static_cast<signed_size_t>(partitionSize)
                ? arg.getChunk(static_cast<std::size_t>(centerOffset - partitionSize) / CHUNK_SIZE)
                : ChunkPtr<ElementType>::null();
        ChunkPtr<ElementType> rightChunk = hasRightChunk && centerOffset >= 0
                ? arg.getChunk(static_cast<std::size_t>(centerOffset) / CHUNK_SIZE)
                : ChunkPtr<ElementType>::null();

        return this->constructChunk([centerOffset, leftChunk = std::move(leftChunk), rightChunk = std::move(rightChunk)](ComplexType *dst, unsigned int computedCount) -> unsigned int {
            assert(computedCount == 0);

            // This is an all-or-nothing transform
            assert(hasRightChunk || !rightChunk.has());
            if (hasRightChunk && rightChunk.has() && rightChunk->getComputedCount() <= static_cast<std::size_t>(centerOffset + endOffsetFromSplit - 1) % CHUNK_SIZE) {
                return 0;
            }
            assert(!leftChunk.has() || centerOffset >= 0);
            if (leftChunk.has() && leftChunk->getComputedCount() <= static_cast<std::size_t>(centerOffset - 1) % CHUNK_SIZE) {
                return 0;
            }

            typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();
            doFft(dst, leftChunk, rightChunk, centerOffset, planIO.real);
            FftwPlanner<ElementType>::release(planIO);

            return fftSize;
        });
    }

    static void doFft(ComplexType *dst, const ChunkPtr<ElementType> &leftChunk, const ChunkPtr<ElementType> &rightChunk, signed_size_t centerOffset, ElementType *scratch) {
#ifndef NDEBUG
        // Make sure we're going to fill everything
        std::fill_n(scratch, fftSize, ElementType(NAN));
#endif

        static constexpr unsigned int dstSplit = dstOffset - beginOffsetFromSplit;

        assert(leftChunk.has() == (centerOffset >= static_cast<signed_size_t>(partitionSize)));
        if (leftChunk.has()) {
            zeroRange<0, dstOffset>(scratch);

            static constexpr unsigned int size = std::min<signed int>(-beginOffsetFromSplit, copySize);
            static_assert(size > 0, "Left chunk's size is not positive");
            static_assert(size <= partitionSize, "Left chunk's size is too big");

            unsigned int srcChunkOffset = (centerOffset + beginOffsetFromSplit) % CHUNK_SIZE;
            for (unsigned int i = 0; i < size; i++) {
                ElementType val = leftChunk->getElement(srcChunkOffset + i);
                scratch[dstOffset + i] = std::isnan(val) ? 0.0 : val * factor;
            }

            zeroRange<dstOffset + size, dstSplit>(scratch);
        } else {
            zeroRange<0, dstSplit>(scratch);
        }

        assert(rightChunk.has() == (hasRightChunk && centerOffset >= 0));
        if (hasRightChunk && rightChunk.has()) {
            static constexpr unsigned int size = endOffsetFromSplit;
            static_assert(size < copySize, "Unexpected endOffsetFromSplit; some of the range should have been taken up by the left chunk");
            static_assert(!hasRightChunk || size > 0, "Right chunk's size is not positive");
            static_assert(size <= partitionSize, "Right chunk's size is too big");

            unsigned int srcChunkOffset = static_cast<std::size_t>(centerOffset) % CHUNK_SIZE;
            for (std::size_t i = 0; i < size; i++) {
                ElementType val = rightChunk->getElement(srcChunkOffset + i);
                scratch[dstSplit + i] = std::isnan(val) ? 0.0 : val * factor;
            }

            zeroRange<dstSplit + size, fftSize>(scratch);
        } else {
            zeroRange<dstSplit, fftSize>(scratch);
        }

#ifndef NDEBUG
        if (leftChunk.has() && hasRightChunk && rightChunk.has()) {
            bool isSameChunks = leftChunk.operator->() == rightChunk.operator->();
            bool shouldBeSame = static_cast<std::size_t>(centerOffset + beginOffsetFromSplit) / CHUNK_SIZE == static_cast<std::size_t>(centerOffset) / CHUNK_SIZE;
            assert(isSameChunks == shouldBeSame);
        }
#endif

#ifndef NDEBUG
        // Make sure we filled everything
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
        std::fill_n(dst + begin, end - begin, ElementType(0.0));
    }
};

}

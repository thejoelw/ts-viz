#pragma once

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "series/type/fftseries.h"
#include "series/invalidparameterexception.h"

#include "defs/CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_USE_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_KERNEL_FFT_PADDING_TYPE.h"
#include "defs/CONV_TS_FFT_PADDING_TYPE.h"
#include "defs/CONV_ASSERT_CORRECTNESS.h"
#include "defs/CONV_CHECK_OPTIMAL.h"

namespace {

static unsigned int exactLog2(std::size_t x) {
    assert(x == 1 || x % 2 == 0);
    return x == 1 ? 0 : 1 + exactLog2(x / 2);
}

template <template <unsigned int> typename ClassType, unsigned int count>
class DispatchToTemplate {
public:
    template <typename ReturnType, typename... ArgTypes>
    static ReturnType call(unsigned int value, ArgTypes... args) {
        assert(value < count);
        return detail<ReturnType, ArgTypes...>::call(value, std::forward<ArgTypes>(args)..., std::make_index_sequence<count>{});
    }

private:
    template <typename ReturnType, typename... ArgTypes>
    struct detail {
        template <std::size_t... is>
        static ReturnType call(unsigned int value, ArgTypes... args, std::index_sequence<is...>) {
            static constexpr std::array<ReturnType (*)(ArgTypes...), count> ptrs = { &dispatch<is>... };
            return ptrs[value](std::forward<ArgTypes>(args)...);
        }

        template <std::size_t value>
        static ReturnType dispatch(ArgTypes... args) {
            return ClassType<value>::call(std::forward<ArgTypes>(args)...);
        }
    };
};

/*
template <template <unsigned int> typename ClassType, unsigned int count>
class DispatchToTemplate;

template <template <unsigned int> typename ClassType>
class DispatchToTemplate<ClassType, 0> {
public:
    template <typename ReturnType, typename... ArgTypes>
    static ReturnType call(unsigned int value, ArgTypes &&...) {
        (void) value;
        assert(false);
    }
};

template <template <unsigned int> typename ClassType, unsigned int count>
class DispatchToTemplate {
public:
    template <typename ReturnType, typename... ArgTypes>
    static ReturnType call(unsigned int value, ArgTypes... args) {
        if (value == count - 1) {
            return ClassType<count - 1>::call(std::forward<ArgTypes>(args)...);
        } else {
            return DispatchToTemplate<ClassType, count - 1>::template call<ReturnType, ArgTypes...>(value, std::forward<ArgTypes>(args)...);
        }
    }
};
*/

}

namespace series {

template <typename ElementType, unsigned int partitionSize>
class KernelFft : public FftSeries<ElementType, partitionSize, CONV_KERNEL_FFT_PADDING_TYPE, std::ratio<1, partitionSize * 2>> {};

template <typename ElementType, unsigned int partitionSize>
struct TsFft : public FftSeries<ElementType, partitionSize, CONV_TS_FFT_PADDING_TYPE> {};

template <typename ElementType>
auto getKernelPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg) {
    static constexpr unsigned int begin = CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2;
    static constexpr unsigned int end = CHUNK_SIZE_LOG2 + 1;
    static_assert(begin <= end, "CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 must be less than or equal to CHUNK_SIZE_LOG2 + 1");
    return getKernelPartitionFfts<ElementType, begin, end>(context, arg, std::make_index_sequence<end - begin>{});
}
template <typename ElementType, std::size_t begin, std::size_t end, std::size_t... is>
auto getKernelPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg, std::index_sequence<is...>) {
#define NUMS (1u << (begin + is))
    return std::tuple<std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>, 2>...>(getFftArr(KernelFft<ElementType, NUMS>::create(context, arg), 0, std::make_index_sequence<2>{})...);
#undef NUMS
}

template <typename ElementType>
auto getTsPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t offset) {
    static constexpr unsigned int begin = CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2;
    static constexpr unsigned int end = CHUNK_SIZE_LOG2 + 1;
    static_assert(begin <= end, "CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 must be less than or equal to CHUNK_SIZE_LOG2 + 1");
    return getTsPartitionFfts<ElementType, begin, end>(context, arg, offset, std::make_index_sequence<end - begin>{});
}
template <typename ElementType, std::size_t begin, std::size_t end, std::size_t... is>
auto getTsPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t offset, std::index_sequence<is...>) {
#define NUMS (1u << (begin + is))
    return std::tuple<std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>, CHUNK_SIZE / NUMS>...>(getFftArr(TsFft<ElementType, NUMS>::create(context, arg), offset, std::make_index_sequence<CHUNK_SIZE / NUMS>{})...);
#undef NUMS
}

template <typename ElementType, std::size_t partitionSize, PaddingType paddingType, typename scale, std::size_t... is>
auto getFftArr(FftSeries<ElementType, partitionSize, paddingType, scale> &fft, std::size_t offset, std::index_sequence<is...>) {
    assert(offset % partitionSize == 0);

    return std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, partitionSize * 2>, sizeof...(is)>{
        fft.template getChunk<partitionSize * 2>(offset / partitionSize + is)...
    };
}


#if CONV_ASSERT_CORRECTNESS
void updateIncludesKernelAfter(unsigned int &ika, unsigned int kernelSize, unsigned int from, unsigned int to) {
    if (to > kernelSize) {
        to = kernelSize;
    }
    if (from > kernelSize) {
        from = kernelSize;
    }

    assert(ika == to);
    ika = from;
}

template <typename ElementType>
void checkCorrect(
        const std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx_impl<ElementType>::Complex, CHUNK_SIZE * 2>>> &kernelChunks,
        const std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx_impl<ElementType>::Complex, CHUNK_SIZE * 2>>> &tsChunks,
        ElementType *data,
        unsigned int elementIndex,
        unsigned int kernelOffset,
        unsigned int kernelEnd
) {
    assert(kernelEnd < kernelChunks.size() * CHUNK_SIZE);

    double sum = 0.0;
    for (unsigned int i = kernelOffset; i < kernelEnd; i++) {
        unsigned int ki = i;
        ElementType kv = kernelChunks[ki / CHUNK_SIZE].first->getElement(ki % CHUNK_SIZE);

        std::pair<unsigned int, unsigned int> ti = getTsIndex(elementIndex - i);
        if(ti.first >= tsChunks.size()) {
            break;
        }
        ElementType tv = tsChunks[ti.first].first->getElement(ti.second);

        sum += kv * tv;
    }

    static constexpr ElementType allowedError = 1e-9;
    ElementType expected = sum;
    ElementType actual = data[elementIndex];

    ElementType dist = std::fabs(expected - actual);
    if (!std::isnan(dist)) {
        int a = 0;
    }
    bool valid = std::isnan(dist)
            ? std::isnan(expected) == std::isnan(actual) || kernelOffset != 0
            : dist <= std::max(static_cast<ElementType>(1.0), std::fabs(expected) + std::fabs(actual)) * allowedError;
    assert(valid);
}
#endif

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    typedef fftwx_impl<ElementType> fftwx;
    typedef typename fftwx::Complex ComplexType;

public:
    ConvSeries(app::AppContext &context, DataSeries<ElementType> &kernel, DataSeries<ElementType> &ts, std::int64_t kernelSize, bool backfillZeros)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelSize(kernelSize)
        , backfillZeros(backfillZeros)
    {
        if (kernelSize <= 1) {
            throw series::InvalidParameterException("ConvSeries: kernelSize must be greater than one");
        }
        if (kernelSize > std::numeric_limits<unsigned int>::max()) {
            throw series::InvalidParameterException("ConvSeries: kernelSize must not be greater than " + std::to_string(std::numeric_limits<unsigned int>::max()));
        }

        FftwPlanner<ElementType>::init();
    }

    ~ConvSeries() {}

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        auto &kernelFft = KernelFft<ElementType, CHUNK_SIZE>::create(this->context, kernel);
        auto &tsFft = TsFft<ElementType, CHUNK_SIZE>::create(this->context, ts);

        // 1 -> 1
        // 2 -> 1
        // 16 -> 1
        // 17 -> 2
        unsigned int numKernelChunks = (kernelSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
        assert(numKernelChunks * CHUNK_SIZE >= kernelSize);
        std::vector<ChunkPtr<ElementType>> kernelChunks;
        kernelChunks.reserve(numKernelChunks);
        for (unsigned int i = 0; i < numKernelChunks; i++) {
            kernelChunks.emplace_back(kernel.getChunk(i));
        }

        // 1 -> 1
        // 2 -> 2
        unsigned int numTsChunks = std::min<unsigned int>((kernelSize + CHUNK_SIZE - 2) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<ChunkPtr<ElementType>> tsChunks;
        tsChunks.reserve(numTsChunks);
        for (unsigned int i = 0; i < numTsChunks; i++) {
            tsChunks.emplace_back(ts.getChunk(chunkIndex + 1 - numTsChunks + i));
        }

//        auto kernelPartitionFfts = getKernelPartitionFfts<ElementType>(this->context, kernel);
//        auto tsPartitionedFfts = getTsPartitionFfts<ElementType>(this->context, ts, chunkIndex * CHUNK_SIZE);

        unsigned int nanEnd = !backfillZeros && kernelSize - 1 > chunkIndex * CHUNK_SIZE ? kernelSize - 1 - chunkIndex * CHUNK_SIZE : 0;

        unsigned int checkProgress = 0;

#if CONV_ASSERT_CORRECTNESS
        std::vector<unsigned int> includesKernelAfter;
        includesKernelAfter.resize(CHUNK_SIZE, kernelSize);
#endif

        return this->constructChunk([
            this,
            chunkIndex,
            kernelChunks = std::move(kernelChunks),
            tsChunks = std::move(tsChunks),
//            kernelPartitionFfts = std::move(kernelPartitionFfts),
//            tsPartitionedFfts = std::move(tsPartitionedFfts),
            nanEnd,
            checkProgress
#if CONV_ASSERT_CORRECTNESS
            , includesKernelAfter = std::move(includesKernelAfter)
            , chunkIndex
#endif
        ](ElementType *dst, unsigned int computedCount) mutable -> unsigned int {
            unsigned int endCount = tsChunks.back()->getComputedCount();
            if (endCount == computedCount) {
                return computedCount;
            }

            switch (checkProgress) {
            case 0:
                for (unsigned int i = 0; i < tsChunks.size() - 1; i++) {
                    if (tsChunks[i]->getComputedCount() != CHUNK_SIZE) {
                        assert(computedCount == 0);
                        return 0;
                    }
                }

                checkProgress++;
                // Fall-through intentional

            case 1:
                for (const ChunkPtr<ElementType> &kc : kernelChunks) {
                    if (kc->getComputedCount() != CHUNK_SIZE) {
                        assert(computedCount == 0);
                        return 0;
                    }
                }

                checkProgress++;
                // Fall-through intentional

            case 2:
                break;
            }

            while (true) {
                ConvParams cp = prepareNextConv(chunkIndex * CHUNK_SIZE + computedCount, chunkIndex * CHUNK_SIZE + endCount);
                if (cp.size == 0) {
                    break;
                }

                std::vector<std::vector<std::pair<unsigned int, unsigned int>>> output = cp.getOutput(kernelSize);
                assert(output.size() == cp.outputEnd - cp.outputStart);

                for (unsigned int m = 0; m < output.size(); m++) {
                    assert(cp.outputStart + m < existingMuls.size());
                    std::vector<std::pair<unsigned int, unsigned int>> &existing = existingMuls[cp.outputStart + m];
                    std::vector<std::pair<unsigned int, unsigned int>> &insert = output[m];
                    existing.insert(existing.end(), insert.cbegin(), insert.cend());

                    std::size_t targ = std::min<std::size_t>(cp.outputStart + m + 1, kernelSize);
                    assert(existing.size() <= targ);
                    if (chunkIndex * CHUNK_SIZE + computedCount == cp.outputStart + m && existing.size() == targ) {
                        computedCount++;
                    }
                }

                DispatchToTemplate<ConvDoer, CHUNK_SIZE_LOG2 + 1>::template call<void>(
                            exactLog2(cp.size),
                            cp,
                            chunkIndex * CHUNK_SIZE,
                            dst,
                            std::cref(kernelChunks),
                            (chunkIndex + 1 - tsChunks.size()) * CHUNK_SIZE,
                            std::cref(tsChunks)
                );
            }

            return computedCount;



            /*
            if (computedCount == 0) {
                // TODO: See if this segfaults?
//                typename fftwx::Complex test[CHUNK_SIZE * 2];

                std::pair<unsigned int, unsigned int> ti = getTsIndex(1u - kernelSize);
                assert(ti.first > 0);

                for (unsigned int i = 1; i < tsChunks.size(); i++) {
                    unsigned int end = i < ti.first ? 0 : (i == ti.first ? ti.second : CHUNK_SIZE);
                    for (unsigned int j = CHUNK_SIZE; j-- > end;) {
                        if (std::isnan(tsChunks[i].first->getElement(j))) {
                            nanEnd = j - i * CHUNK_SIZE + kernelSize;
                            goto foundNan;
                        }
                    }
                }
                foundNan:

                unsigned int len = std::min(kernelChunks.size(), tsChunks.size());
                if (len > 1) {
                    const typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();

                    for (unsigned int i = len; i-- > 1;) {
                        ElementType *res = finishConv<ElementType, CHUNK_SIZE>(planIO, kernelChunks[i].second->getData(), tsChunks[i].second->getData());

                        if (i == len - 1) {
                            // Don't really need to do this here right now, but I think it could be useful in the future
    //                        assert(nanEnd <= CHUNK_SIZE);
    //                        std::fill_n(dst, nanEnd, NAN);
    //                        std::copy(planIO.out + CHUNK_SIZE + nanEnd, planIO.out + CHUNK_SIZE * 2, dst + nanEnd);

                            std::copy_n(res, CHUNK_SIZE, dst);
                        } else {
                            for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
                                dst[i] += res[i];
                            }
                        }

#if CONV_ASSERT_CORRECTNESS
                        for (unsigned int j = 0; j < CHUNK_SIZE; j++) {
                            updateIncludesKernelAfter(includesKernelAfter[j], kernelSize, i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE);

                            checkCorrect(kernelChunks, tsChunks, dst, j, i * CHUNK_SIZE, kernelSize);
                        }
#endif
                    }

                    FftwPlanner<ElementType>::release(planIO);
                } else {
                    assert(len > 0);
                    std::fill_n(dst, CHUNK_SIZE, ElementType(0.0));
                }

                // The chunk initialization could have taken awhile, so go ahead and make sure we're processing the maximum number of samples
                unsigned int newEndCount = tsChunks[0].first->getComputedCount();
                assert(newEndCount >= endCount);
                endCount = newEndCount;
            }

            for (unsigned int i = computedCount; i < endCount; i++) {
                if (std::isnan(tsChunks[0].first->getElement(i))) {
                    assert(i + kernelSize > nanEnd);
                    nanEnd = i + kernelSize;
                }
                if (i < nanEnd) {
                    dst[i] = NAN;
                }
            }

            // 15 -> 3
            // 16 -> 3 (because a 4 would try convolving K[16:32], which is all zero
            // 17 -> 4
            assert(kernelSize != 1);
            unsigned int maxBegin = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(kernelSize - 1);

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


                // 10010100 <- prev
                // 10110110 <- targ
                // 00000100 <- step

                // 10011000 <- prev
                // 10110110 <- targ
                // 00001000 <- step

                // 10100000 <- prev
                // 10110110 <- targ
                // 00010000 <- step

                // 10110000 <- prev
                // 10110110 <- targ
                // 00000100 <- step

                // 10110100 <- prev
                // 10110110 <- targ
                // 00000010 <- step

                unsigned int countLsz = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(computedCount ^ (computedCount - 1));
                unsigned int maxStepLog2 = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(endCount - computedCount);
                unsigned int stepLog2 = std::min(countLsz, maxStepLog2);
                unsigned int start = std::min(countLsz, maxBegin);
//                unsigned int start = std::min<unsigned int>(countLsz, CHUNK_SIZE_LOG2);

                for (signed int i = start; i >= static_cast<signed int>(stepLog2); i--) {
                    // Check to make sure we have all the data we need

                    bool res = DispatchToTemplate<ParitionChecker, CHUNK_SIZE_LOG2 + 1>::template call<bool>(i, computedCount, std::cref(kernelPartitionFfts), std::cref(tsPartitionedFfts), 1);
                    if (!res) {
                        return computedCount;
                    }
                }

                bool res = DispatchToTemplate<ParitionChecker, CHUNK_SIZE_LOG2 + 1>::template call<bool>(stepLog2, computedCount, std::cref(kernelPartitionFfts), std::cref(tsPartitionedFfts), 0);
                if (!res) {
                    return computedCount;
                }

                for (signed int i = start; i >= static_cast<signed int>(stepLog2); i--) {
                    DispatchToTemplate<PartitionFiller, CHUNK_SIZE_LOG2 + 1>::template call<void>(
                                i, dst, computedCount, std::cref(kernelChunks[0].first), std::cref(tsChunks[0].first), std::cref(kernelPartitionFfts), std::cref(tsPartitionedFfts), 1
#if CONV_ASSERT_CORRECTNESS
                                , std::cref(kernelChunks), std::cref(tsChunks), kernelSize, std::ref(includesKernelAfter)
#endif
                            );
                }

                DispatchToTemplate<PartitionFiller, CHUNK_SIZE_LOG2 + 1>::template call<void>(
                            stepLog2, dst, computedCount, std::cref(kernelChunks[0].first), std::cref(tsChunks[0].first), std::cref(kernelPartitionFfts), std::cref(tsPartitionedFfts), 0
#if CONV_ASSERT_CORRECTNESS
                            , std::cref(kernelChunks), std::cref(tsChunks), kernelSize, std::ref(includesKernelAfter)
#endif
                        );

#if CONV_ASSERT_CORRECTNESS
                for (unsigned int i = 0; i < (1u << stepLog2); i++) {
                    assert(includesKernelAfter[computedCount + i] == 0);
                }
#endif

                computedCount += 1u << stepLog2;
            }

            assert(computedCount == endCount);

            return computedCount;
            */
        });
    }

#if CONV_CHECK_OPTIMAL
    struct ConvParams {
        unsigned int size;
        unsigned int kernelStart;
        unsigned int kernelEnd;
        unsigned int tsStart;
        unsigned int tsEnd;
        unsigned int resultStart;
        unsigned int resultEnd;
        unsigned int outputStart;
        unsigned int outputEnd;

        void assertValid() const {
            assert(kernelStart % size == 0);
            assert(kernelEnd % size == 0);
            assert(tsStart % size == 0);
            assert(tsEnd % size == 0);
            assert(resultStart % size == 0);
            assert(resultEnd % size == 0);
            assert(outputStart % size == 0);
            assert(outputEnd % size == 0);

            assert(kernelEnd - kernelStart == size || kernelEnd - kernelStart == size * 2);
            assert(tsEnd - tsStart == size || tsEnd - tsStart == size * 2);
            assert(resultEnd - resultStart == size || resultEnd - resultStart == size * 2);
            assert(outputEnd - outputStart == resultEnd - resultStart);

            assert(outputStart == tsStart + kernelStart + resultStart);
            assert(outputEnd == tsStart + kernelStart + resultEnd);
            assert(outputEnd - outputStart == resultEnd - resultStart);
        }

        std::vector<std::vector<std::pair<unsigned int, unsigned int>>> getOutput(std::size_t kernelSize) const {
            std::vector<std::vector<std::pair<unsigned int, unsigned int>>> res;
            for (unsigned int i = resultStart; i < resultEnd; i++) {
                res.emplace_back();
                for (unsigned int j = 0; j < size * 2; j++) {
                    unsigned int k = j;
                    unsigned int t = (i + size * 2 - j) % (size * 2);
                    if (kernelStart + k < kernelSize && k < kernelEnd - kernelStart && t < tsEnd - tsStart) {
                        res.back().emplace_back(kernelStart + k, tsStart + t);
                    }
                }
            }
            return res;
        }
    };

    ConvParams prepareNextConv(unsigned int from, unsigned int to) {
        // Assumption: Generated convs' [tsStart : tsEnd] intersect [from : to]

        for (unsigned int size = CHUNK_SIZE; size > 0; size /= 2) {
            unsigned int tsStart = from / size * size;
            if (tsStart > 0) {
                tsStart -= size;
            }
            unsigned int tsEnd = to / size * size;

            for (unsigned int resultType = 3; resultType-- > 0;) {
                for (unsigned int kernelType = 2; kernelType >= 1; kernelType--) {
                    for (unsigned int tsType = 2; tsType >= 1; tsType--) {
                        for (unsigned int i = tsStart; i < tsEnd; i += size) {
                            for (unsigned int j = 0; j < kernelSize; j += size) {
                                ConvParams cp;
                                cp.size = size;
                                cp.kernelStart = j;
                                cp.kernelEnd = j + size * kernelType;
                                cp.tsStart = i;
                                cp.tsEnd = i + size * tsType;
                                if (size == CHUNK_SIZE && resultType == 2) {
                                    continue;
                                }
//                                if (resultType == 2) {
//                                    continue;
//                                }
                                switch (resultType) {
                                    case 0: cp.resultStart = 0; cp.resultEnd = size; break;
                                    case 1: cp.resultStart = size; cp.resultEnd = size * 2; break;
                                    case 2: cp.resultStart = 0; cp.resultEnd = size * 2; break;
                                }
                                cp.outputStart = cp.tsStart + cp.kernelStart + cp.resultStart;
                                cp.outputEnd = cp.tsStart + cp.kernelStart + cp.resultEnd;

                                cp.assertValid();

                                if (cp.tsEnd > to) {
                                    // We don't have the data for this
                                    continue;
                                }

                                if (cp.outputStart >= to) {
                                    // No need to go into the future yet
                                    continue;
                                }

                                std::vector<std::vector<std::pair<unsigned int, unsigned int>>> output = cp.getOutput(kernelSize);
                                assert(output.size() == cp.outputEnd - cp.outputStart);

                                std::size_t count = 0;
                                for (unsigned int m = 0; m < output.size(); m++) {
                                    while (existingMuls.size() <= cp.outputStart + m) {
                                        existingMuls.emplace_back();
                                    }
                                    std::vector<std::pair<unsigned int, unsigned int>> &existing = existingMuls[cp.outputStart + m];
                                    std::vector<std::pair<unsigned int, unsigned int>> &insert = output[m];

                                    for (auto y : insert) {
                                        if (y.second > cp.outputStart + m) {
                                            // Can't know the future
                                            goto nextCandidate;
                                        }

                                        for (auto x : existing) {
                                            if (x.first == y.first || x.second == y.second) {
                                                assert(x.first == y.first && x.second == y.second);
                                                goto nextCandidate;
                                            }
                                        }
                                    }

                                    count += insert.size();
                                }

                                if (count) {
                                    // Good candidate!
                                    return cp;
                                }

                                nextCandidate:;
                            }
                        }
                    }
                }
            }
        }

        ConvParams res;
        res.size = 0;
        return res;
    }

    template <unsigned int sizeLog2>
    struct ConvDoer {
        static void call(
                ConvParams cp,
                unsigned int dstOffset,
                ElementType *dst,
                const std::vector<ChunkPtr<ElementType>> &kernelChunks,
                unsigned int tsOffset,
                const std::vector<ChunkPtr<ElementType>> &tsChunks
        ) {
            static_assert(sizeLog2 <= CHUNK_SIZE_LOG2, "We should not be generating code to handle larger-than-chunk-size fills");

            static constexpr unsigned int size = 1u << sizeLog2;
            assert(cp.size == size);

            const typename FftwPlanner<ElementType>::IO planIO_0(fftwx::alloc_real(CHUNK_SIZE * 2), fftwx::alloc_complex(CHUNK_SIZE * 2));
            const typename FftwPlanner<ElementType>::IO planIO_1(fftwx::alloc_real(CHUNK_SIZE * 2), fftwx::alloc_complex(CHUNK_SIZE * 2));
            typename fftwx::Plan planFwd = FftwPlanner<ElementType>::template getPlanFwd<size * 2>();

            cp.assertValid();

            assert(cp.kernelStart < cp.kernelEnd);
            assert(cp.kernelEnd <= kernelChunks.size() * CHUNK_SIZE);
            for (std::size_t i = cp.kernelStart; i < cp.kernelEnd; i++) {
                ElementType val = kernelChunks[i / CHUNK_SIZE]->getElement(i % CHUNK_SIZE);
                planIO_0.real[i - cp.kernelStart] = val * (ElementType(0.5) / size);
            }
            std::fill(planIO_0.real + cp.kernelEnd - cp.kernelStart, planIO_0.real + size * 2, ElementType(0.0));
            fftwx::execute_dft_r2c(planFwd, planIO_0.real, planIO_0.complex);

            assert(cp.tsStart < cp.tsEnd);
            assert(cp.tsStart >= tsOffset);
            assert(cp.tsEnd <= tsOffset + tsChunks.size() * CHUNK_SIZE);
            assert(tsOffset % CHUNK_SIZE == 0);
            for (std::size_t i = cp.tsStart; i < cp.tsEnd; i++) {
                ElementType val = tsChunks[(i - tsOffset) / CHUNK_SIZE]->getElement(i % CHUNK_SIZE);
                planIO_1.real[i - cp.tsStart] = std::isnan(val) ? 0.0 : val;
            }
            std::fill(planIO_1.real + cp.tsEnd - cp.tsStart, planIO_1.real + size * 2, ElementType(0.0));
            typename fftwx::Plan plan_1 = FftwPlanner<ElementType>::template getPlanFwd<size * 2>();
            fftwx::execute_dft_r2c(planFwd, planIO_1.real, planIO_1.complex);

            assert(cp.resultStart < cp.resultEnd);
            assert(cp.resultEnd <= size * 2);
            finishConv<size>(planIO_0, planIO_0.complex, planIO_1.complex);

            assert(cp.outputStart < cp.outputEnd);
            assert(cp.outputStart >= dstOffset);
            assert(cp.outputEnd <= dstOffset + CHUNK_SIZE);
            assert(cp.outputEnd - cp.outputStart == cp.resultEnd - cp.resultStart);
            for (unsigned int i = 0; i < cp.outputEnd - cp.outputStart; i++) {
                dst[cp.outputStart + i - dstOffset] += planIO_0.real[cp.resultStart + i];
            }

            fftwx::free(planIO_0.real);
            fftwx::free(planIO_0.complex);
            fftwx::free(planIO_1.real);
            fftwx::free(planIO_1.complex);
        }
    };
#endif

    template <unsigned int fillSizeLog2>
    struct ParitionChecker {
        static bool call(
                unsigned int computedCount,
                const typename std::result_of<decltype(&getKernelPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &)>::type &kernelPartitionFfts,
                const typename std::result_of<decltype(&getTsPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &, std::size_t)>::type &tsPartitionedFfts,
                bool kernelPartitionIndex // Either the first or second partition
        ) {
            static_assert(fillSizeLog2 <= CHUNK_SIZE_LOG2, "We should not be generating code to handle larger-than-chunk-size fills");

            static constexpr unsigned int fillSize = 1u << fillSizeLog2;
            assert(computedCount % fillSize == 0);

            if (kernelPartitionIndex && computedCount == 0) {
                return true;
            }

            if constexpr (fillSizeLog2 >= CONV_USE_FFT_ABOVE_SIZE_LOG2) {
                if constexpr (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    const ChunkPtr<typename fftwx::Complex, fillSize * 2> &kc = std::get<std::array<ChunkPtr<typename fftwx::Complex, fillSize * 2>, 2>>(kernelPartitionFfts)[kernelPartitionIndex];
                    if (kc->getComputedCount() != fillSize * 2) {
                        return false;
                    }
                }

                if constexpr (fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                    unsigned int tsOffset = computedCount - fillSize * kernelPartitionIndex;
                    assert(computedCount >= fillSize * kernelPartitionIndex);

                    assert(tsOffset / fillSize < CHUNK_SIZE / fillSize);
                    const ChunkPtr<typename fftwx::Complex, fillSize * 2> &tc = std::get<std::array<ChunkPtr<typename fftwx::Complex, fillSize * 2>, CHUNK_SIZE / fillSize>>(tsPartitionedFfts)[tsOffset / fillSize];
                    if (tc->getComputedCount() != fillSize * 2) {
                        return false;
                    }
                }
            }

            return true;
        }
    };

    template <unsigned int fillSizeLog2>
    struct PartitionFiller {
        static void call(
                ElementType *dst,
                unsigned int computedCount,
                const ChunkPtr<ElementType> &kernelChunk,
                const ChunkPtr<ElementType> &tsChunk,
                const typename std::result_of<decltype(&getKernelPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &)>::type &kernelPartitionFfts,
                const typename std::result_of<decltype(&getTsPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &, std::size_t)>::type &tsPartitionedFfts,
                bool kernelPartitionIndex // Either the first or second partition
#if CONV_ASSERT_CORRECTNESS
                , const std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx_impl<ElementType>::Complex, CHUNK_SIZE * 2>>> &kernelChunks
                , const std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx_impl<ElementType>::Complex, CHUNK_SIZE * 2>>> &tsChunks
                , unsigned int kernelSize
                , std::vector<unsigned int> &includesKernelAfter
#endif
        ) {
            static_assert(fillSizeLog2 <= CHUNK_SIZE_LOG2, "We should not be generating code to handle larger-than-chunk-size fills");

            static_assert((sizeof(ElementType) << CONV_USE_FFT_ABOVE_SIZE_LOG2) >= 16, "Data subscripts won't be aligned to 16-byte boundaries");

            static constexpr unsigned int fillSize = 1u << fillSizeLog2;
            assert(computedCount % fillSize == 0);

            if (kernelPartitionIndex && computedCount == 0) {
                return;
            }

            unsigned int kernelOffset = fillSize * kernelPartitionIndex;
            unsigned int tsOffset = computedCount - fillSize * kernelPartitionIndex;
            assert(computedCount >= fillSize * kernelPartitionIndex);

            if constexpr (true || fillSizeLog2 >= CONV_USE_FFT_ABOVE_SIZE_LOG2) {
                const typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();

                for (unsigned int j = 0; j < 2; j++) {
                    const typename fftwx::Complex *kernelFft;
                    if constexpr (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                        const ChunkPtr<typename fftwx::Complex, fillSize * 2> &kc = std::get<std::array<ChunkPtr<typename fftwx::Complex, fillSize * 2>, 2>>(kernelPartitionFfts)[kernelPartitionIndex];
                        assert(kc->getComputedCount() == fillSize * 2);
                        kernelFft = kc->getData();
                    } else {
                        kernelFft = planIO.complex;

                        ChunkPtr<ElementType> np = ChunkPtr<ElementType>::null();
                        switch (CONV_KERNEL_FFT_PADDING_TYPE) {
                        case PaddingType::ZeroFill:
                            KernelFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(kernelFft), np, 0, kernelChunk, kernelOffset, planIO.real);
                            break;
                        case PaddingType::PriorChunk:
                            KernelFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(kernelFft), kernelOffset ? kernelChunk : np, kernelOffset - fillSize, kernelChunk, kernelOffset, planIO.real);
                            break;
                        }
                    }

                    const typename fftwx::Complex *tsFft;
                    if constexpr (fillSizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                        assert(tsOffset / fillSize < CHUNK_SIZE / fillSize);
                        const ChunkPtr<typename fftwx::Complex, fillSize * 2> &tc = std::get<std::array<ChunkPtr<typename fftwx::Complex, fillSize * 2>, CHUNK_SIZE / fillSize>>(tsPartitionedFfts)[tsOffset / fillSize];
                        assert(tc->getComputedCount() == fillSize * 2);
                        tsFft = tc->getData();
                    } else {
                        if constexpr (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                            tsFft = planIO.complex;
                        } else {
                            static_assert(fillSize * 4 <= CHUNK_SIZE * 2, "Not enough room in the PlanIO for two ffts. This probably means CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 and CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 are too big.");
                            tsFft = planIO.complex + fillSize * 2;
                        }

                        ChunkPtr<ElementType> np = ChunkPtr<ElementType>::null();
                        switch (CONV_TS_FFT_PADDING_TYPE) {
                        case PaddingType::ZeroFill:
                            TsFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(tsFft), np, 0, tsChunk, tsOffset, planIO.real);
                            break;
                        case PaddingType::PriorChunk:
                            TsFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(tsFft), tsOffset ? tsChunk : np, tsOffset - fillSize, tsChunk, tsOffset, planIO.real);
                            break;
                        }
                    }

                    ElementType *res = finishConv<ElementType, fillSize>(planIO, kernelFft, tsFft);

                    for (unsigned int i = 0; i < fillSize; i++) {
                        /*
                        if (computedCount + i == 1210) {
                            unsigned int sizeLog2 = fillSizeLog2;
                            unsigned int size = fillSize;
                            ElementType prev = dst[computedCount + i];
                            ElementType add = res[fillSize + i];
                            int a = 123;
                        }
                        */
                        dst[computedCount + i] += res[i];
                    }
                }

                FftwPlanner<ElementType>::release(planIO);
            } else {
                assert(fillSizeLog2 < CHUNK_SIZE_LOG2);

                for (unsigned int i = 0; i < fillSize; i++) {
                    for (unsigned int j = 0; j < fillSize; j++) {
                        /*
                        ElementType k;
                        switch (CONV_KERNEL_FFT_PADDING_TYPE) {
                        case PaddingType::ZeroFill:
                            k = kernelChunk->getElement(kernelOffset + j);
                            KernelFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(kernelFft), np, 0, kernelChunk, kernelOffset, planIO.real);
                            break;
                        case PaddingType::PriorChunk:
                            KernelFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(kernelFft), kernelOffset ? kernelChunk : np, kernelOffset - fillSize, kernelChunk, kernelOffset, planIO.real);
                            break;
                        }

                        ElementType t;
                        switch (CONV_TS_FFT_PADDING_TYPE) {
                        case PaddingType::ZeroFill:
                            TsFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(tsFft), np, 0, tsChunk, tsOffset, planIO.real);
                            break;
                        case PaddingType::PriorChunk:
                            TsFft<ElementType, fillSize>::doFft(const_cast<decltype(planIO.complex)>(tsFft), tsOffset ? tsChunk : np, tsOffset - fillSize, tsChunk, tsOffset, planIO.real);
                            break;
                        }

                        ElementType k = kernelChunk->getElement(kernelOffset + j);
                        ElementType t = tsChunk->getElement(tsOffset + i - j);
                        dst[computedCount + i] += k * t;
                        */
                    }
                }
            }

#if CONV_ASSERT_CORRECTNESS
            for (unsigned int i = 0; i < fillSize; i++) {
                updateIncludesKernelAfter(includesKernelAfter[computedCount + i], kernelSize, kernelOffset, kernelOffset + fillSize);

                checkCorrect(kernelChunks, tsChunks, dst, computedCount + i, kernelOffset, kernelSize);
            }
#endif
        }
    };

    template <unsigned int size>
    static ElementType *finishConv(const typename FftwPlanner<ElementType>::IO planIO, const typename fftwx_impl<ElementType>::Complex *a, const typename fftwx_impl<ElementType>::Complex *b) {
        for (unsigned int i = 0; i < size * 2; i++) {
            planIO.complex[i] = a[i] * b[i];
        }

        typename fftwx_impl<ElementType>::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<size * 2>();
        fftwx_impl<ElementType>::execute_dft_c2r(planBwd, planIO.complex, planIO.real);

        return planIO.real;
    }

private:
    DataSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    unsigned int kernelSize;
    bool backfillZeros;

#if CONV_CHECK_OPTIMAL
    static_assert(ENABLE_CHUNK_MULTITHREADING == 0, "We don't support multithreading when checking optimal convolutions yet.");
    std::vector<std::vector<std::pair<unsigned int, unsigned int>>> existingMuls;
#endif
};

}

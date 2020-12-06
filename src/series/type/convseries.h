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

namespace {

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

template <typename ElementType>
auto getKernelPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t chunkIndex) {
    static constexpr unsigned int begin = CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2;
    static constexpr unsigned int end = CHUNK_SIZE_LOG2 + 1;
    static_assert(begin <= end, "CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 must be less than or equal to CHUNK_SIZE_LOG2 + 1");
    return getKernelPartitionFfts<ElementType, begin, end>(context, arg, chunkIndex, std::make_index_sequence<end - begin>{});
}
template <typename ElementType, std::size_t begin, std::size_t end, std::size_t... is>
auto getKernelPartitionFfts(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t chunkIndex, std::index_sequence<is...>) {
#define NUMS (1u << (begin + is))
    return std::tuple<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>...>(KernelFft<ElementType, NUMS>::create(context, arg).template getChunk<NUMS * 2>(chunkIndex)...);
#undef NUMS
}

template <typename ElementType, unsigned int partitionSize>
struct TsFft : public FftSeries<ElementType, partitionSize, CONV_TS_FFT_PADDING_TYPE> {};

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
    return std::tuple<std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, NUMS * 2>, CHUNK_SIZE / NUMS>...>(getTsFftArr(TsFft<ElementType, NUMS>::create(context, arg), offset)...);
#undef NUMS
}
template <typename ElementType, std::size_t partitionSize>
auto getTsFftArr(FftSeries<ElementType, partitionSize, CONV_TS_FFT_PADDING_TYPE> &tsFft, std::size_t offset) {
    return getTsFftArr(tsFft, offset, std::make_index_sequence<CHUNK_SIZE / partitionSize>{});
}
template <typename ElementType, std::size_t partitionSize, std::size_t... is>
auto getTsFftArr(FftSeries<ElementType, partitionSize, CONV_TS_FFT_PADDING_TYPE> &tsFft, std::size_t offset, std::index_sequence<is...>) {
    assert(offset % partitionSize == 0);

    return std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, partitionSize * 2>, sizeof...(is)>{
        tsFft.template getChunk<partitionSize * 2>(offset / partitionSize + is)...
    };
}

inline std::pair<unsigned int, unsigned int> getTsIndex(signed int index) {
    static_assert(static_cast<signed int>(-1) >> CHUNK_SIZE_LOG2 == static_cast<signed int>(-1), "Signed right-shift doesn't do sign extension! :(");
    assert(static_cast<signed int>(-1) >> CHUNK_SIZE_LOG2 == static_cast<signed int>(-1));

    return std::pair<unsigned int, unsigned int>(-(index >> CHUNK_SIZE_LOG2), static_cast<unsigned int>(index) % CHUNK_SIZE);
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
    return;

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
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<ComplexType, CHUNK_SIZE * 2>>> kernelChunks;
        kernelChunks.reserve(numKernelChunks);
        for (unsigned int i = 0; i < numKernelChunks; i++) {
            kernelChunks.emplace_back(kernel.getChunk(i), kernelFft.template getChunk<CHUNK_SIZE * 2>(i));
        }

        // 1 -> 1
        // 2 -> 2
        unsigned int numTsChunks = std::min<unsigned int>((kernelSize + CHUNK_SIZE - 2) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<ComplexType, CHUNK_SIZE * 2>>> tsChunks;
        tsChunks.reserve(numTsChunks);
        for (unsigned int i = 0; i < numTsChunks; i++) {
            tsChunks.emplace_back(ts.getChunk(chunkIndex - i), tsFft.template getChunk<CHUNK_SIZE * 2>(chunkIndex - i));
        }

        auto kernelPartitionFfts0 = getKernelPartitionFfts<ElementType>(this->context, kernel, 0);
        auto kernelPartitionFfts1 = getKernelPartitionFfts<ElementType>(this->context, kernel, 1);

        auto tsPartitionedFfts = getTsPartitionFfts<ElementType>(this->context, ts, chunkIndex * CHUNK_SIZE);

        unsigned int nanEnd = !backfillZeros && kernelSize - 1 > chunkIndex * CHUNK_SIZE ? kernelSize - 1 - chunkIndex * CHUNK_SIZE : 0;

        unsigned int checkProgress = 0;

#if CONV_ASSERT_CORRECTNESS
        std::vector<unsigned int> includesKernelAfter;
        includesKernelAfter.resize(CHUNK_SIZE, kernelSize);
#endif

        return this->constructChunk([
            this,
            kernelChunks = std::move(kernelChunks),
            tsChunks = std::move(tsChunks),
            kernelPartitionFfts0 = std::move(kernelPartitionFfts0),
            kernelPartitionFfts1 = std::move(kernelPartitionFfts1),
            tsPartitionedFfts = std::move(tsPartitionedFfts),
            nanEnd,
            checkProgress
#if CONV_ASSERT_CORRECTNESS
            , includesKernelAfter = std::move(includesKernelAfter)
            , chunkIndex
#endif
        ](ElementType *dst, unsigned int computedCount) mutable -> unsigned int {
            unsigned int endCount = tsChunks[0].first->getComputedCount();
            if (endCount == computedCount) {
                return computedCount;
            }

            switch (checkProgress) {
            case 0:
                for (unsigned int i = 1; i < tsChunks.size(); i++) {
                    if (tsChunks[i].second->getComputedCount() != CHUNK_SIZE * 2) {
                        assert(computedCount == 0);
                        return 0;
                    }
                    assert(tsChunks[i].first->getComputedCount() == CHUNK_SIZE);
                }

                checkProgress++;
                // Fall-through intentional

            case 1:
                for (std::pair<ChunkPtr<ElementType>, ChunkPtr<ComplexType, CHUNK_SIZE * 2>> &kc : kernelChunks) {
                    if (kc.second->getComputedCount() != CHUNK_SIZE * 2) {
                        assert(computedCount == 0);
                        return 0;
                    }
                    assert(kc.first->getComputedCount() == CHUNK_SIZE);
                }

                checkProgress++;
                // Fall-through intentional

            case 2:
                break;
            }


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
                        // Elementwise multiply
                        for (unsigned int j = 0; j < CHUNK_SIZE * 2; j++) {
                            planIO.complex[j] = kernelChunks[i].second->getElement(j) * tsChunks[i].second->getElement(j);
                        }

                        typename fftwx::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<CHUNK_SIZE * 2>();
                        fftwx::execute_dft_c2r(planBwd, planIO.complex, planIO.real);

                        if (i == len - 1) {
                            // Don't really need to do this here right now, but I think it could be useful in the future
    //                        assert(nanEnd <= CHUNK_SIZE);
    //                        std::fill_n(dst, nanEnd, NAN);
    //                        std::copy(planIO.out + CHUNK_SIZE + nanEnd, planIO.out + CHUNK_SIZE * 2, dst + nanEnd);

                            std::copy(planIO.real, planIO.real + CHUNK_SIZE, dst);
                        } else {
                            for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
                                dst[i] += planIO.real[i];
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
                unsigned int maxStepLog2 = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(endCount - computedCount);
                unsigned int stepLog2 = std::min(countLsz, maxStepLog2);
                unsigned int start = std::min(countLsz, maxBegin);
//                unsigned int start = std::min<unsigned int>(countLsz, CHUNK_SIZE_LOG2);

                for (signed int i = start; i >= static_cast<signed int>(stepLog2); i--) {
                    // Check to make sure we have all the data we need

                    bool res = DispatchToTemplate<ParitionChecker, CHUNK_SIZE_LOG2 + 1>::template call<bool>(i, computedCount, std::cref(kernelPartitionFfts1), std::cref(tsPartitionedFfts), 1);
                    if (!res) {
                        return computedCount;
                    }
                }

                bool res = DispatchToTemplate<ParitionChecker, CHUNK_SIZE_LOG2 + 1>::template call<bool>(stepLog2, computedCount, std::cref(kernelPartitionFfts0), std::cref(tsPartitionedFfts), 0);
                if (!res) {
                    return computedCount;
                }

                for (signed int i = start; i >= static_cast<signed int>(stepLog2); i--) {
                    DispatchToTemplate<PartitionFiller, CHUNK_SIZE_LOG2 + 1>::template call<void>(
                                i, dst, computedCount, std::cref(kernelChunks[0].first), std::cref(tsChunks[0].first), std::cref(kernelPartitionFfts1), std::cref(tsPartitionedFfts), 1
#if CONV_ASSERT_CORRECTNESS
                                , std::cref(kernelChunks), std::cref(tsChunks), kernelSize, std::ref(includesKernelAfter)
#endif
                            );
                }

                DispatchToTemplate<PartitionFiller, CHUNK_SIZE_LOG2 + 1>::template call<void>(
                            stepLog2, dst, computedCount, std::cref(kernelChunks[0].first), std::cref(tsChunks[0].first), std::cref(kernelPartitionFfts0), std::cref(tsPartitionedFfts), 0
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
        });
    }

    template <unsigned int fillSizeLog2>
    struct ParitionChecker {
        static bool call(
                unsigned int computedCount,
                const typename std::result_of<decltype(&getKernelPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &, std::size_t)>::type &kernelPartitionFfts,
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
                    const ChunkPtr<typename fftwx::Complex, fillSize * 2> &kc = std::get<ChunkPtr<typename fftwx::Complex, fillSize * 2>>(kernelPartitionFfts);
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
                const typename std::result_of<decltype(&getKernelPartitionFfts<ElementType>)(app::AppContext &, DataSeries<ElementType> &, std::size_t)>::type &kernelPartitionFfts,
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

                const typename fftwx::Complex *kernelFft;
                if constexpr (fillSizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                    const ChunkPtr<typename fftwx::Complex, fillSize * 2> &kc = std::get<ChunkPtr<typename fftwx::Complex, fillSize * 2>>(kernelPartitionFfts);
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

                // Elementwise multiply
                for (std::size_t i = 0; i < fillSize * 2; i++) {
                    planIO.complex[i] = kernelFft[i] * tsFft[i];
                }

                // Backward fft
                typename fftwx::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<fillSize * 2>();
                fftwx::execute_dft_c2r(planBwd, planIO.complex, planIO.real);

                for (unsigned int i = 0; i < fillSize; i++) {
                    /*
                    if (computedCount + i == 1210) {
                        unsigned int sizeLog2 = fillSizeLog2;
                        unsigned int size = fillSize;
                        ElementType prev = dst[computedCount + i];
                        ElementType add = planIO.real[fillSize + i];
                        int a = 123;
                    }
                    */
                    dst[computedCount + i] += planIO.real[i];
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

private:
    DataSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    unsigned int kernelSize;
    bool backfillZeros;
};

}

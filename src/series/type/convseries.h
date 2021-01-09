#pragma once

#include "series/dataseries.h"
#include "series/fftwx.h"
#include "series/type/fftseries.h"
#include "series/invalidparameterexception.h"
#include "util/uniquetuple.h"

#include "defs/CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_USE_FFT_ABOVE_SIZE_LOG2.h"
#include "defs/CONV_VARIANT.h"

#include "series/type/convvariant/zpts1.h"
#include "series/type/convvariant/zpts2.h"
#include "series/type/convvariant/zpkernel.h"
typedef CONV_VARIANT ConvVariant;

namespace {

template <typename Type>
struct Wrapper {};

}

namespace series {

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
        std::size_t offset = chunkIndex * CHUNK_SIZE;

        auto kernelPartitionFfts = std::apply([this](auto... stepSpecs) {
            typedef typename util::BuildUniqueTuple<std::tuple<>, std::tuple<Wrapper<typename decltype(stepSpecs)::template KernelFft<ElementType>>...>>::type KernelWrappers;
            return std::apply([this](auto... wrappers) {
                return std::tuple_cat(getKernelPartitionFfts(this->context, kernel, wrappers)...);
            }, KernelWrappers());
        }, ConvVariant::makeStepSpecSpace());

        auto tsPartitionFfts = std::apply([this, offset](auto... stepSpecs) {
            typedef typename util::BuildUniqueTuple<std::tuple<>, std::tuple<Wrapper<typename decltype(stepSpecs)::template TsFft<ElementType>>...>>::type TsWrappers;
            return std::apply([this, offset](auto... wrappers) {
                return std::tuple_cat(getTsPartitionFfts(this->context, ts, offset, wrappers)...);
            }, TsWrappers());
        }, ConvVariant::makeStepSpecSpace());

        auto &kernelFft = ConvVariant::PriorChunkStepSpec::KernelFft<ElementType>::create(this->context, kernel);
        auto &tsFft = ConvVariant::PriorChunkStepSpec::TsFft<ElementType>::create(this->context, ts);

        // 1 -> 1
        // 2 -> 1
        // 16 -> 1
        // 17 -> 2
        unsigned int numKernelChunks = (kernelSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
        assert(numKernelChunks * CHUNK_SIZE >= kernelSize);
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx::Complex, CHUNK_SIZE * 2>>> kernelChunks;
        kernelChunks.reserve(numKernelChunks);
        for (unsigned int i = 0; i < numKernelChunks; i++) {
            signed int ki = i;
            kernelChunks.emplace_back(kernel.getChunk(ki), kernelFft.template getChunk<CHUNK_SIZE * 2>(ki));
        }

        // 1 -> 1
        // 2 -> 2
        unsigned int numTsChunks = std::min<unsigned int>((kernelSize + CHUNK_SIZE - 2) / CHUNK_SIZE, chunkIndex) + 1;
        std::vector<std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx::Complex, CHUNK_SIZE * 2>>> tsChunks;
        tsChunks.reserve(numTsChunks);
        for (unsigned int i = 0; i < numTsChunks; i++) {
            signed int ti = chunkIndex + 1 - numTsChunks + i;
            assert(ti >= 0);
            tsChunks.emplace_back(ts.getChunk(ti), tsFft.template getChunk<CHUNK_SIZE * 2>(ti));
        }

        unsigned int nanEnd = !backfillZeros && kernelSize - 1 > chunkIndex * CHUNK_SIZE ? kernelSize - 1 - chunkIndex * CHUNK_SIZE : 0;

        unsigned int checkProgress = 0;

        return this->constructChunk([
            this,
            chunkIndex,
            kernelChunks = std::move(kernelChunks),
            tsChunks = std::move(tsChunks),
            kernelPartitionFfts = std::move(kernelPartitionFfts),
            tsPartitionFfts = std::move(tsPartitionFfts),
            nanEnd,
            checkProgress
        ](ElementType *dst, unsigned int computedCount) mutable -> unsigned int {
            unsigned int endCount = tsChunks.back().first->getComputedCount();
            if (endCount == computedCount) {
                return computedCount;
            }

            switch (checkProgress) {
            case 0:
                for (unsigned int i = 0; i < tsChunks.size() - 1; i++) {
                    if (tsChunks[i].first->getComputedCount() != CHUNK_SIZE) {
                        assert(computedCount == 0);
                        return 0;
                    } else {
                        assert(tsChunks[i].second->getComputedCount() == CHUNK_SIZE * 2);
                    }
                }

                checkProgress++;
                // Fall-through intentional

            case 1:
                for (const std::pair<ChunkPtr<ElementType>, ChunkPtr<typename fftwx::Complex, CHUNK_SIZE * 2>> &kc : kernelChunks) {
                    if (kc.first->getComputedCount() != CHUNK_SIZE) {
                        assert(computedCount == 0);
                        return 0;
                    } else {
                        assert(kc.second->getComputedCount() == CHUNK_SIZE * 2);
                    }
                }

                checkProgress++;
                // Fall-through intentional

            case 2:
                break;
            }

            if (computedCount == 0) {
                assert(tsChunks.size() > 0);
                unsigned int start = (tsChunks.size() - 1) * CHUNK_SIZE; // Don't include the last chunk
                unsigned int end = start > kernelSize ? start - kernelSize : 0;
                for (unsigned int i = start; i-- > end;) {
                    assert(i / CHUNK_SIZE < tsChunks.size() - 1);
                    if (std::isnan(tsChunks[i / CHUNK_SIZE].first->getElement(i % CHUNK_SIZE))) {
                        unsigned int newNanEnd = start - i + kernelSize - 1;
                        assert(newNanEnd > nanEnd);
                        nanEnd = newNanEnd;
                        goto foundNan;
                    }
                }
                foundNan:

                unsigned int len = std::min(kernelChunks.size(), tsChunks.size()) - 1;
                assert(len != static_cast<unsigned int>(-1));
                if (len > 0) {
                    const typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();

                    for (unsigned int i = 0; i < len; i++) {
                        util::DispatchToLambda<bool, 2>::call(i == 0, [dst, planIO, &kernelChunks, &tsChunks, i](auto isFirstTag) {
                            unsigned int ti = i;
                            unsigned int ki = tsChunks.size() - 1 - ti;

                            ConvVariant::PriorChunkStepSpec stepSpec;
                            static_assert(stepSpec.fftSizeLog2 == CHUNK_SIZE_LOG2 + 1, "Incorrect fftSizeLog2");

                            assert(ki > 0);
                            assert(ki < kernelChunks.size());
                            const ChunkPtr<typename fftwx::Complex, CHUNK_SIZE * 2> &kc = kernelChunks[ki].second;
                            assert(kc->getComputedCount() == CHUNK_SIZE * 2);
                            const typename fftwx::Complex *kernelFft = kc->getData();

                            assert(ti < tsChunks.size());
                            const ChunkPtr<typename fftwx::Complex, CHUNK_SIZE * 2> &tc = tsChunks[ti].second;
                            assert(tc->getComputedCount() == CHUNK_SIZE * 2);
                            const typename fftwx::Complex *tsFft = tc->getData();

                            // Elementwise multiply
                            for (unsigned int i = 0; i < CHUNK_SIZE * 2; i++) {
                                planIO.complex[i] = kernelFft[i] * tsFft[i];
                            }

                            typename fftwx_impl<ElementType>::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<CHUNK_SIZE * 2>();
                            fftwx_impl<ElementType>::execute_dft_c2r(planBwd, planIO.complex, planIO.real);

                            static_assert(stepSpec.resultSize == CHUNK_SIZE, "Unexpected ssw.resultSize");
                            static_assert(stepSpec.dstSize == CHUNK_SIZE, "Unexpected ssw.dstSize");
                            for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
                                unsigned int srcIndex = stepSpec.resultBegin + i;
                                assert(srcIndex < CHUNK_SIZE * 2);

                                unsigned int dstIndex = stepSpec.dstOffsetFromCc + i;
                                assert(dstIndex < CHUNK_SIZE);

                                if constexpr (isFirstTag.value) {
                                    dst[dstIndex] = planIO.real[srcIndex];
                                } else {
                                    dst[dstIndex] += planIO.real[srcIndex];
                                }
                            }
                        });
                    }

                    FftwPlanner<ElementType>::release(planIO);
                } else {
                    std::fill_n(dst, CHUNK_SIZE, ElementType(0.0));
                }

                // The chunk initialization could have taken awhile, so go ahead and make sure we're processing the maximum number of samples possible
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
            assert(kernelSize > 1);
            unsigned int maxBegin = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(kernelSize - 1);

            while (computedCount < endCount) {
                ConvVariant::withStepSpec(computedCount, endCount, [dst, &computedCount, &kernelChunks, &tsChunks, &kernelPartitionFfts, &tsPartitionFfts](auto stepSpec) {
                    typedef typename decltype(stepSpec)::template KernelFft<ElementType> KernelFft;
                    typedef typename decltype(stepSpec)::template TsFft<ElementType> TsFft;

                    if constexpr (true || stepSpec.fftSizeLog2 > CONV_USE_FFT_ABOVE_SIZE_LOG2) {
                        const typename FftwPlanner<ElementType>::IO planIO = FftwPlanner<ElementType>::request();

                        static constexpr unsigned int kernelIndex = stepSpec.kernelIndex / stepSpec.strideSize;
                        const typename fftwx::Complex *kernelFft;
                        if constexpr (stepSpec.fftSizeLog2 > CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                            static_assert(stepSpec.kernelIndex % stepSpec.strideSize == 0, "The kernel index must be a multiple of the stride size");
                            static_assert(kernelIndex < 2, "KernelIndex is too big! We'd need to prepare more chunks for this.");
                            // If std::get fails to compile because of duplicate types, this probably means there are distinct kernel FftSeries that generate the same ChunkPtr type.
                            // This isn't necessarily unworkable, but it probably means more stuff will be computed than need be.
                            const ChunkPtr<typename fftwx::Complex, stepSpec.fftSize> &kc = std::get<std::array<ChunkPtr<typename fftwx::Complex, stepSpec.fftSize>, 2>>(kernelPartitionFfts)[kernelIndex];
                            assert(kc->getComputedCount() == stepSpec.fftSize);
                            kernelFft = kc->getData();
                        } else {
                            static_assert(stepSpec.strideSize < CHUNK_SIZE, "CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 is too big");
                            static constexpr std::make_signed<std::size_t>::type centerOffset = kernelIndex * stepSpec.strideSize + KernelFft::splitOffset;

                            kernelFft = planIO.complex;
                            ChunkPtr<ElementType> np = ChunkPtr<ElementType>::null();
                            KernelFft::doFft(
                                const_cast<decltype(planIO.complex)>(kernelFft),
                                centerOffset >= stepSpec.strideSize ? kernelChunks.front().first : np,
                                KernelFft::hasRightChunk && centerOffset >= 0 ? kernelChunks.front().first : np,
                                centerOffset,
                                planIO.real
                            );
                        }

                        unsigned int tsOffset = computedCount + stepSpec.tsIndexOffsetFromCc;
                        assert(tsOffset % stepSpec.strideSize == 0);
                        const typename fftwx::Complex *tsFft;
                        if constexpr (stepSpec.fftSizeLog2 > CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
                            unsigned int tsIndex = tsOffset / stepSpec.strideSize;
                            assert(tsIndex < CHUNK_SIZE / stepSpec.strideSize);
                            // If std::get fails to compile because of duplicate types, this probably means there are distinct TS FftSeries that generate the same ChunkPtr type.
                            // This isn't necessarily unworkable, but it probably means more stuff will be computed than need be.
                            const ChunkPtr<typename fftwx::Complex, stepSpec.fftSize> &tc = std::get<std::array<ChunkPtr<typename fftwx::Complex, stepSpec.fftSize>, CHUNK_SIZE / stepSpec.strideSize>>(tsPartitionFfts)[tsIndex];
                            assert(tc->getComputedCount() == stepSpec.fftSize);
                            tsFft = tc->getData();
                        } else {
                            static_assert(stepSpec.strideSize < CHUNK_SIZE, "CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 is too big");
                            std::make_signed<std::size_t>::type centerOffset = tsOffset + TsFft::splitOffset;

                            if constexpr (stepSpec.fftSizeLog2 > CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
                                tsFft = planIO.complex;
                            } else {
                                static_assert(stepSpec.fftSize * 2 <= CHUNK_SIZE * 2, "Not enough room in the PlanIO for two ffts. This probably means CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2 and CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2 are too big.");
                                tsFft = planIO.complex + stepSpec.fftSize;
                            }

                            ChunkPtr<ElementType> np = ChunkPtr<ElementType>::null();
                            TsFft::doFft(
                                const_cast<decltype(planIO.complex)>(tsFft),
                                centerOffset >= stepSpec.strideSize ? tsChunks.back().first : np,
                                TsFft::hasRightChunk && centerOffset >= 0 ? tsChunks.back().first : np,
                                centerOffset,
                                planIO.real
                            );
                        }

                        // Elementwise multiply
                        for (unsigned int i = 0; i < stepSpec.fftSize; i++) {
                            planIO.complex[i] = kernelFft[i] * tsFft[i];
                        }

                        typename fftwx_impl<ElementType>::Plan planBwd = FftwPlanner<ElementType>::template getPlanBwd<stepSpec.fftSize>();
                        fftwx_impl<ElementType>::execute_dft_c2r(planBwd, planIO.complex, planIO.real);

                        static_assert(stepSpec.resultSize == stepSpec.dstSize, "stepSpec.resultSize != stepSpec.dstSize");
                        static constexpr unsigned int loopSplit = std::min(stepSpec.resultSize, stepSpec.fftSize - stepSpec.resultBegin);

                        for (unsigned int i = 0; i < loopSplit; i++) {
                            unsigned int srcIndex = stepSpec.resultBegin + i;
                            assert(srcIndex < stepSpec.fftSize);

                            unsigned int dstIndex = computedCount + stepSpec.dstOffsetFromCc + i;
                            assert(dstIndex < CHUNK_SIZE);

                            dst[dstIndex] += planIO.real[srcIndex];
                        }

                        for (unsigned int i = loopSplit; i < stepSpec.dstSize; i++) {
                            unsigned int srcIndex = stepSpec.resultBegin - stepSpec.fftSize + i;
                            assert(srcIndex < stepSpec.fftSize);

                            unsigned int dstIndex = computedCount + stepSpec.dstOffsetFromCc + i;
                            assert(dstIndex < CHUNK_SIZE);

                            dst[dstIndex] += planIO.real[srcIndex];
                        }

                        FftwPlanner<ElementType>::release(planIO);
                    } else {
                        assert(false);
                    }

                    computedCount += stepSpec.computedIncrement;
                });
            }

            assert(computedCount == endCount);

            return computedCount;
        });
    }

private:
    DataSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    unsigned int kernelSize;
    bool backfillZeros;
};


template <std::size_t x>
constexpr unsigned int exactLog2() {
    static_assert(x == 1 || x % 2 == 0, "Non-power-of-2");
    return x == 1 ? 0 : 1 + exactLog2<x / 2>();
}

template <typename ElementType, std::size_t partitionSize, signed int srcOffset, unsigned int copySize, unsigned int dstOffset>
auto getKernelPartitionFfts(app::AppContext &context, DataSeries<ElementType> &kernel, Wrapper<FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset, std::ratio<1, partitionSize * 2>>>) {
    static constexpr unsigned int sizeLog2 = exactLog2<partitionSize>();
    if constexpr (sizeLog2 >= CONV_USE_FFT_ABOVE_SIZE_LOG2 && sizeLog2 >= CONV_CACHE_KERNEL_FFT_ABOVE_SIZE_LOG2) {
#ifndef NDEBUG
        static bool printed = false;
        if (!printed) {
            spdlog::info("For kernel, requesting {} fft chunks of partitionSize={}, srcOffset={}, copySize={}, dstOffset={}", 2, partitionSize, srcOffset, copySize, dstOffset);
            printed = true;
        }
#endif
        return std::make_tuple(getFftArr(FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset, std::ratio<1, partitionSize * 2>>::create(context, kernel), 0, std::make_index_sequence<2>{}));
    } else {
        return std::tuple<>();
    }
}

template <typename ElementType, std::size_t partitionSize, signed int srcOffset, unsigned int copySize, unsigned int dstOffset>
auto getTsPartitionFfts(app::AppContext &context, DataSeries<ElementType> &ts, std::size_t offset, Wrapper<FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset>>) {
    static constexpr unsigned int sizeLog2 = exactLog2<partitionSize>();
    if constexpr (sizeLog2 >= CONV_USE_FFT_ABOVE_SIZE_LOG2 && sizeLog2 >= CONV_CACHE_TS_FFT_ABOVE_SIZE_LOG2) {
#ifndef NDEBUG
        static bool printed = false;
        if (!printed) {
            spdlog::info("For ts, requesting {} fft chunks of partitionSize={}, srcOffset={}, copySize={}, dstOffset={}", CHUNK_SIZE / partitionSize, partitionSize, srcOffset, copySize, dstOffset);
            printed = true;
        }
#endif
        return std::make_tuple(getFftArr(FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset>::create(context, ts), offset, std::make_index_sequence<CHUNK_SIZE / partitionSize>{}));
    } else {
        return std::tuple<>();
    }
}

template <typename ElementType, std::size_t partitionSize, signed int srcOffset, unsigned int copySize, unsigned int dstOffset, typename scale, std::size_t... is>
static auto getFftArr(FftSeries<ElementType, partitionSize, srcOffset, copySize, dstOffset, scale> &fft, std::size_t offset, std::index_sequence<is...>) {
    assert(offset % partitionSize == 0);

    return std::array<ChunkPtr<typename fftwx_impl<ElementType>::Complex, partitionSize * 2>, sizeof...(is)>{
        fft.template getChunk<partitionSize * 2>(offset / partitionSize + is)...
    };
}

}

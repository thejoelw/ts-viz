#pragma once

#include "series/type/fftseries.h"

namespace series {

template <typename BaseType>
struct StepSpecWrapper : public BaseType {
    template <typename... ArgTypes>
    StepSpecWrapper(ArgTypes... args)
        : BaseType(std::forward<ArgTypes>(args)...)
    {}

    static constexpr unsigned int fftSize = 1u << BaseType::fftSizeLog2;
    static constexpr unsigned int strideSize = fftSize / 2;

    static_assert(BaseType::resultBegin >= 0, "Having a resultBegin less than zero doesn't make sense");
    static_assert(BaseType::dstOffsetFromCc >= 0, "Cannot change values that have already been emitted");
    static_assert(BaseType::resultSize == BaseType::dstSize, "The result size we are using must be the same size as the destination size");
    static_assert(BaseType::resultSize > 0, "The result size must be positive");

    static_assert(BaseType::kernelIndex % strideSize == 0, "kernelIndex must be a multiple of the stride size");
    static_assert(BaseType::tsOffsetFromIndex % strideSize == 0, "tsOffsetFromIndex must be a multiple of the stride size");

    static_assert(BaseType::tsIndexOffsetFromCc + BaseType::tsOffsetFromIndex + BaseType::tsSize <= BaseType::computedIncrement, "Trying to use TS elements that aren't necessarily available");

    static_assert(BaseType::tsIndexOffsetFromCc + BaseType::tsOffsetFromIndex + BaseType::kernelIndex + BaseType::kernelOffsetFromIndex + BaseType::resultBegin == BaseType::dstOffsetFromCc, "Incorrect placement");

    template <typename ElementType>
    using KernelFft = FftSeries<ElementType, strideSize, BaseType::kernelOffsetFromIndex, BaseType::kernelSize, 0, std::ratio<1, fftSize>>;
    template <typename ElementType>
    using TsFft = FftSeries<ElementType, strideSize, BaseType::tsOffsetFromIndex, BaseType::tsSize, 0>;
};

}

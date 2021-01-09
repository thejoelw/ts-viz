#pragma once

#include <assert.h>
#include <algorithm>

#include "series/chunksize.h"
#include "series/type/helper/stepspecwrapper.h"
#include "util/dispatch2lambda.h"
#include "util/constexprcontrol.h"

namespace series {
namespace convvariant {

// Doesn't work; reads too far forwards in the TS series

struct ZpTs2 {
private:
    template <unsigned int sizeLog2, bool incDstSize>
    struct StepSpecDiag {
        static constexpr unsigned int fftSizeLog2 = sizeLog2 + 1;
        static constexpr unsigned int computedIncrement = 1;

        static constexpr unsigned int kernelIndex = 1u << sizeLog2; // Doesn't cause additional FftSeries to be generated. Index must be aligned.
        static constexpr signed int kernelOffsetFromIndex = -(1u << sizeLog2);
        static constexpr unsigned int kernelSize = 2u << sizeLog2;

        static constexpr signed int tsIndexOffsetFromCc = 1 - (1u << sizeLog2); // Doesn't cause additional FftSeries to be generated. Index must be aligned.
        static constexpr signed int tsOffsetFromIndex = 0;
        static constexpr unsigned int tsSize = 1u << sizeLog2;

        static constexpr signed int resultBegin = (1u << sizeLog2) - 1;
        static constexpr unsigned int resultSize = (1u << sizeLog2) + incDstSize;

        static constexpr signed int dstOffsetFromCc = 0;
        static constexpr unsigned int dstSize = (1u << sizeLog2) + incDstSize;
    };

    template <unsigned int sizeLog2>
    struct StepSpecTri {
        static constexpr unsigned int fftSizeLog2 = sizeLog2 + 1;
        static constexpr unsigned int computedIncrement = (1u << sizeLog2) - 1;

        static constexpr unsigned int kernelIndex = 0; // Doesn't cause additional FftSeries to be generated. Index must be aligned.
        static constexpr signed int kernelOffsetFromIndex = -(1u << sizeLog2);
        static constexpr unsigned int kernelSize = 2u << sizeLog2;

        static constexpr signed int tsIndexOffsetFromCc = 0; // Doesn't cause additional FftSeries to be generated. Index must be aligned.
        static constexpr signed int tsOffsetFromIndex = 0;
        static constexpr unsigned int tsSize = (1u << sizeLog2) - 1;

        static constexpr signed int resultBegin = 1u << sizeLog2;
        static constexpr unsigned int resultSize = (1u << sizeLog2) - 1;

        static constexpr signed int dstOffsetFromCc = 0;
        static constexpr unsigned int dstSize = (1u << sizeLog2) - 1;
    };

public:
    typedef StepSpecWrapper<StepSpecDiag<CHUNK_SIZE_LOG2, 0>> PriorChunkStepSpec;

    static auto makeStepSpecSpace() {
        return util::constexprFlatFor(std::make_index_sequence<CHUNK_SIZE_LOG2>{}, [](auto indexTag) {
            static constexpr std::size_t index = indexTag.value;
            return std::make_tuple(
                StepSpecWrapper<StepSpecDiag<index, 0>>(),
                StepSpecWrapper<StepSpecDiag<index, 1>>(),
                StepSpecWrapper<StepSpecTri<index>>()
            );
        });
    }

    template <typename HandlerType>
    static void withStepSpec(unsigned int computedCount, unsigned int endCount, HandlerType handler) {
        unsigned int lsb = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(computedCount ^ (computedCount - 1));
        if (lsb == 0) {
            unsigned int fillSizeLog2 = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz((computedCount + 1) ^ computedCount);

            util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2>::call(fillSizeLog2, [computedCount, handler = std::forward<HandlerType>(handler)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                if (computedCount == CHUNK_SIZE - (1u << sizeLog2) - 1) {
                    handler(StepSpecWrapper<StepSpecDiag<sizeLog2, 1>>());
                } else {
                    handler(StepSpecWrapper<StepSpecDiag<sizeLog2, 0>>());
                }
            });
        } else {
            unsigned int maxStepPlus1Log2 = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(endCount - computedCount + 1);
            assert(maxStepPlus1Log2 > 0);
            unsigned int stepPlus1Log2 = std::min(lsb, maxStepPlus1Log2);

            util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2>::call(stepPlus1Log2, [handler = std::forward<HandlerType>(handler)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                handler(StepSpecWrapper<StepSpecTri<sizeLog2>>());
            });
        }
    }
};

}
}

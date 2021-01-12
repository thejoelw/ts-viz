#pragma once

#include <assert.h>
#include <algorithm>

#include "series/chunksize.h"
#include "series/type/helper/stepspecwrapper.h"
#include "util/dispatch2lambda.h"
#include "util/constexprcontrol.h"

namespace series {
namespace convvariant {

struct ZpTs1 {
private:
    template <unsigned int sizeLog2>
    struct StepSpecDiag {
        static constexpr unsigned int fftSizeLog2 = sizeLog2 + 1;
        static constexpr unsigned int computedIncrement = 0;

        static constexpr unsigned int kernelIndex = 1u << sizeLog2; // Doesn't cause additional FftSeries to be generated. Resulting index must be aligned.
        static constexpr signed int kernelOffsetFromIndex = -(1u << sizeLog2);
        static constexpr unsigned int kernelSize = 2u << sizeLog2;

        static constexpr signed int tsIndexOffsetFromCc = -(1u << sizeLog2); // Doesn't cause additional FftSeries to be generated. Resulting index must be aligned.
        static constexpr signed int tsOffsetFromIndex = 0;
        static constexpr unsigned int tsSize = 1u << sizeLog2;

        static constexpr signed int resultBegin = 1u << sizeLog2;
        static constexpr unsigned int resultSize = 1u << sizeLog2;

        static constexpr signed int dstOffsetFromCc = 0;
        static constexpr unsigned int dstSize = 1u << sizeLog2;
    };

    template <unsigned int sizeLog2>
    struct StepSpecTri {
        static constexpr unsigned int fftSizeLog2 = sizeLog2 + 1;
        static constexpr unsigned int computedIncrement = 1u << sizeLog2;

        static constexpr unsigned int kernelIndex = 0; // Doesn't cause additional FftSeries to be generated. Resulting index must be aligned.
        static constexpr signed int kernelOffsetFromIndex = -(1u << sizeLog2);
        static constexpr unsigned int kernelSize = 2u << sizeLog2;

        static constexpr signed int tsIndexOffsetFromCc = 0; // Doesn't cause additional FftSeries to be generated. Resulting index must be aligned.
        static constexpr signed int tsOffsetFromIndex = 0;
        static constexpr unsigned int tsSize = 1u << sizeLog2;

        static constexpr signed int resultBegin = 1u << sizeLog2;
        static constexpr unsigned int resultSize = 1u << sizeLog2;

        static constexpr signed int dstOffsetFromCc = 0;
        static constexpr unsigned int dstSize = 1u << sizeLog2;
    };

public:
    typedef StepSpecWrapper<StepSpecDiag<CHUNK_SIZE_LOG2>> PriorChunkStepSpec;

    static auto makeStepSpecSpace() {
        return std::tuple_cat(
            util::constexprFlatFor(std::make_index_sequence<CHUNK_SIZE_LOG2>{}, [](auto indexTag) {
                return std::make_tuple(StepSpecWrapper<StepSpecDiag<indexTag.value>>());
            }),
            util::constexprFlatFor(std::make_index_sequence<CHUNK_SIZE_LOG2 + 1>{}, [](auto indexTag) {
                return std::make_tuple(StepSpecWrapper<StepSpecTri<indexTag.value>>());
            })
        );
    }

    template <typename CheckerType, typename HandlerType>
    static bool withStepSpec(unsigned int computedCount, unsigned int endCount, CheckerType checker, HandlerType handler) {
        unsigned int countLsz = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(computedCount ^ (computedCount - 1));
        unsigned int maxStepLog2 = sizeof(unsigned int) * CHAR_BIT - 1 - __builtin_clz(endCount - computedCount);
        unsigned int stepLog2 = std::min(countLsz, maxStepLog2);

        if (computedCount > 0) {
            if (!util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2 + 1>::call<bool>(stepLog2, [checker = std::forward<CheckerType>(checker)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                return checker(StepSpecWrapper<StepSpecTri<sizeLog2>>());
            })) { return false; }

            if (!util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2>::call<bool>(countLsz, [checker = std::forward<CheckerType>(checker), handler = std::forward<HandlerType>(handler)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                return checker(StepSpecWrapper<StepSpecDiag<sizeLog2>>()) && (handler(StepSpecWrapper<StepSpecDiag<sizeLog2>>()), true);
            })) { return false; }

            util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2 + 1>::call<void>(stepLog2, [handler = std::forward<HandlerType>(handler)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                handler(StepSpecWrapper<StepSpecTri<sizeLog2>>());
            });

            return true;
        } else {
            return util::DispatchToLambda<unsigned int, CHUNK_SIZE_LOG2 + 1>::call<bool>(stepLog2, [checker = std::forward<CheckerType>(checker), handler = std::forward<HandlerType>(handler)](auto sizeTag) {
                static constexpr unsigned int sizeLog2 = sizeTag.value;
                return checker(StepSpecWrapper<StepSpecTri<sizeLog2>>()) && (handler(StepSpecWrapper<StepSpecTri<sizeLog2>>()), true);
            });
        }
    }
};

}
}

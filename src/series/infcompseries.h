#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType>
class InfCompSeries : public DataSeries<ElementType> {
public:
    InfCompSeries(app::AppContext &context, OperatorType op)
        : DataSeries<ElementType>(context)
        , op(op)
    {}

    std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        return [this, chunkIndex, dst](unsigned int computedCount) -> unsigned int {
            assert(computedCount == 0);

            std::size_t begin = chunkIndex * CHUNK_SIZE;
            std::size_t end = (chunkIndex + 1) * CHUNK_SIZE;
            op(dst, begin, end);

            return CHUNK_SIZE;
        };
    }

private:
    OperatorType op;
};

}

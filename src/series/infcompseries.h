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

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        return [this, chunkIndex](ElementType *dst) {
            std::size_t begin = chunkIndex * CHUNK_SIZE;
            std::size_t end = (chunkIndex + 1) * CHUNK_SIZE;
            op(dst, begin, end);
        };
    }

private:
    OperatorType op;
};

}

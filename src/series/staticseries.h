#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType>
class StaticSeries : public DataSeries<ElementType> {
public:
    StaticSeries(app::AppContext &context, OperatorType op, std::size_t width)
        : DataSeries<ElementType>(context)
        , op(op)
        , width(width)
    {}

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        return [this, chunkIndex](ElementType *dst) {
            op(dst, chunkIndex * DataSeries<ElementType>::ChunkData::size, (chunkIndex + 1) * DataSeries<ElementType>::ChunkData::size);
        };
    }

    std::size_t getStaticWidth() const override {
        return width;
    }

private:
    OperatorType op;
    std::size_t width;
};

}

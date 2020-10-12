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

    std::string getName() const override { return "static"; }

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        return [this, chunkIndex](ElementType *dst) {
            static constexpr std::size_t size = DataSeries<ElementType>::Chunk::size;
            std::size_t begin = chunkIndex * size;
            std::size_t end = (chunkIndex + 1) * size;
            op(dst, begin, end);
        };
    }

private:
    OperatorType op;
};

}

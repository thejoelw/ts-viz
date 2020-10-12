#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType>
class FiniteCompSeries : public DataSeries<ElementType> {
public:
    FiniteCompSeries(app::AppContext &context, OperatorType op, std::size_t width)
        : DataSeries<ElementType>(context)
        , op(op)
        , width(width)
    {}

    std::string getName() const override { return "static"; }

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        return [this, chunkIndex](ElementType *dst) {
            static constexpr std::size_t size = DataSeries<ElementType>::Chunk::size;
            std::size_t begin = chunkIndex * size;
            std::size_t end = (chunkIndex + 1) * size;
            if (width < begin) {
                std::fill_n(dst, size, NAN);
                return;
            } else if (width < end) {
                std::fill(dst + width - begin, dst + size, NAN);
                end = width;
            }
            op(dst, begin, end);
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

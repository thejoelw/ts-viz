#pragma once

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class ParallelOpSeries : public DataSeries<ElementType> {
public:
    ParallelOpSeries(app::AppContext &context, OperatorType op, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , args(args...)
    {}

    fu2::unique_function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return std::move([this, dst, chunks = std::move(chunks)](unsigned int computedCount) -> unsigned int {
            unsigned int count = std::apply([](auto &... x){return std::min({x->getComputedCount()...});}, chunks);
            for (std::size_t i = computedCount; i < count; i++) {
                dst[i] = std::apply([this, i](auto &... s){return op(s->getElement(i)...);}, chunks);
            }
            return count;
        });
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}

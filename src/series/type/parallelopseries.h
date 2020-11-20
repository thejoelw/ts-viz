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

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return this->constructChunk([this, chunks = std::move(chunks)](ElementType *dst, unsigned int computedCount) -> unsigned int {
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

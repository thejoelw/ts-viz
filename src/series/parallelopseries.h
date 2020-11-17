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

    std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return [this, dst, chunks = std::move(chunks)](unsigned int computedCount) {
            unsigned int count = std::apply([](auto ... x){return std::min({x->getComputedCount()...});}, chunks);
            assert(count >= computedCount);
            for (std::size_t i = computedCount; i < count; i++) {
                dst[i] = std::apply([this, i](auto ... s){return op(s->getElement(i)...);}, chunks);
            }
            return count;
        };
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}

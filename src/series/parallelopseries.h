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

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return [this, chunks](ElementType *dst) {
            auto sources = std::apply([](auto ... x){return std::make_tuple(x->getData()...);}, chunks);
            for (std::size_t i = 0; i < CHUNK_SIZE; i++) {
                *dst++ = std::apply([this](auto *&... s){return op(*s++...);}, sources);
            }
        };
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}

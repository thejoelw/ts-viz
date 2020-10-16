#pragma once

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class ScannedSeries : public DataSeries<ElementType> {
public:
    ScannedSeries(app::AppContext &context, OperatorType op, ElementType initialValue, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , prevValue(initialValue)
        , args(args...)
    {}

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        if (chunkIndex > 0) {
            // All we gotta do is set it as a dependency, so the prevValue will be updated.
            this->getChunk(chunkIndex - 1);
        }
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return [this, chunks](ElementType *dst) {
            auto sources = std::apply([](auto ... x){return std::make_tuple(x->getData()...);}, chunks);
            ElementType value = prevValue;
            for (std::size_t i = 0; i < DataSeries<ElementType>::Chunk::size; i++) {
                value = std::apply([this, value](auto *&... s){return op(value, *s++...);}, sources);
                *dst++ = value;
            }
            prevValue = value;
        };
    }

private:
    OperatorType op;

    ElementType prevValue;
    std::tuple<ArgTypes &...> args;
};

}

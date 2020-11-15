#pragma once

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class DeltaSeries : public DataSeries<ElementType> {
public:
    DeltaSeries(app::AppContext &context, OperatorType op, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , args(args...)
    {}

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        typedef typename DataSeries<ElementType>::Chunk Chunk;

        auto prevChunks = std::apply([chunkIndex](auto &... x){
            return std::make_tuple((chunkIndex > 0 ? x.getChunk(chunkIndex - 1) : std::shared_ptr<Chunk>(0))...);
        }, args);
        auto curChunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return [this, chunkIndex, prevChunks = std::move(prevChunks), curChunks = std::move(curChunks)](ElementType *dst) {
            auto curSources = std::apply([](auto ... x){return std::make_tuple(x->getData()...);}, curChunks);
            if (chunkIndex == 0) {
                *dst++ = NAN;
            } else {
                auto prevSources = std::apply([](auto ... x){return std::make_tuple(x->getData() + (CHUNK_SIZE - 1)...);}, prevChunks);
                *dst++ = std::apply([this](auto *... s){return op(*s...);}, std::tuple_cat(curSources, prevSources));
            }
            for (std::size_t i = 1; i < CHUNK_SIZE; i++) {
                *dst++ = std::apply([this](auto *&... s){
                    std::make_tuple(s++...);
                    return op(*s..., *(s - 1)...);
                }, curSources);
            }
        };
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}

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

    std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        typedef typename series::DataSeries<ElementType>::ChunkPtr ChunkPtr;

        auto prevChunks = std::apply([chunkIndex](auto &... x){
            return std::make_tuple((chunkIndex > 0 ? x.getChunk(chunkIndex - 1) : ChunkPtr(0))...);
        }, args);
        auto curChunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return [this, chunkIndex, dst, prevChunks = std::move(prevChunks), curChunks = std::move(curChunks)](unsigned int computedCount) -> unsigned int {
            unsigned int count = std::apply([](auto ... x){return std::min({x->getComputedCount()...});}, curChunks);

            if (count > 0) {
                if (computedCount == 0) {
                    if (chunkIndex == 0) {
                        dst[0] = NAN;
                    } else {
                        bool allPrevComplete = std::apply([](auto ... x) {return (... && (x->getComputedCount() == CHUNK_SIZE));}, prevChunks);
                        if (allPrevComplete) {
                            auto curSources = std::apply([](auto ... x){return std::make_tuple(x->getElement(0)...);}, curChunks);
                            auto prevSources = std::apply([](auto ... x){return std::make_tuple(x->getElement(CHUNK_SIZE - 1)...);}, prevChunks);
                            dst[0] = std::apply([this](auto ... s){return op(s...);}, std::tuple_cat(curSources, prevSources));
                        } else {
                            return 0;
                        }
                    }
                    computedCount++;
                }

                for (std::size_t i = computedCount; i < count; i++) {
                    dst[i] = std::apply([this, i](auto ... x){
                        return op(x->getElement(i)..., x->getElement(i - 1)...);
                    }, curChunks);
                }
            }
            return count;
        };
    }

private:
    OperatorType op;

    std::tuple<ArgTypes &...> args;
};

}

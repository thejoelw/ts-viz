#pragma once

#include "series/dataseries.h"
#include "program/progobj.h"

namespace {

/*
template <typename ElementType>
class ChunkArrayIterator {
public:
    ChunkArrayIterator(const std::vector<typename series::DataSeries<ElementType>::Chunk *> &chunkArr)
        : chunkArr(chunkArr)
    {}

private:
    const std::vector<typename series::DataSeries<ElementType>::Chunk *> chunkArr;
};

template <typename ElementType>
static typename series::DataSeries<ElementType>::Chunk *getChunk(series::DataSeries<ElementType> &s, std::size_t chunkIndex) {
    return s.getChunk(chunkIndex);
}
template <typename ElementType>
static std::vector<typename series::DataSeries<ElementType>::Chunk *> getChunk(program::ProgObjArray<series::DataSeries<ElementType> *> &arr, std::size_t chunkIndex) {
    std::vector<typename series::DataSeries<ElementType>::Chunk *> res;
    for (const series::DataSeries<ElementType> *s : arr.getArr()) {
        res.push_back(s->getChunk(chunkIndex));
    }
    return res;
}

//template <typename ElementType>
//static ElementType *getData(typename series::DataSeries<ElementType>::Chunk *chunk) {
//    return chunk->getData();
//}
static float *getData(typename series::DataSeries<float>::Chunk *chunk) {
    return chunk->getData();
}
static double *getData(typename series::DataSeries<double>::Chunk *chunk) {
    return chunk->getData();
}
template <typename ElementType>
static ChunkArrayIterator<ElementType> getData(const std::vector<typename series::DataSeries<ElementType>::Chunk *> &chunkArr) {
    return ChunkArrayIterator(chunkArr);
}
*/

}

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
        return [this, chunks = std::move(chunks)](ElementType *dst) {
            auto sources = std::apply([](auto ... x){return std::make_tuple(x->getData()...);}, chunks);
            ElementType value = prevValue;
            for (std::size_t i = 0; i < CHUNK_SIZE; i++) {
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

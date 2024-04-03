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
    ScannedSeries(app::AppContext &context, OperatorType op, ElementType initialValue, ArgTypes... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , initialValue(initialValue)
        , args(args...)
    {}

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        auto prevChunk = chunkIndex > 0 ? this->getChunk(chunkIndex - 1) : ChunkPtr<ElementType>::null();
        auto chunks = std::apply([chunkIndex](auto &... x){return std::make_tuple(x.getChunk(chunkIndex)...);}, args);
        return this->constructChunk([this, prevChunk = std::move(prevChunk), chunks = std::move(chunks)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            ElementType value;
            if (computedCount > 0) {
                value = dst[computedCount - 1];
            } else {
                if (prevChunk.has()) {
                    if (prevChunk->getComputedCount() == CHUNK_SIZE) {
                        value = prevChunk->getElement(CHUNK_SIZE - 1);
                    } else {
                        return 0;
                    }
                } else {
                    value = initialValue;
                }
            }

            unsigned int endCount = std::apply([](auto &... x){return std::min({x->getComputedCount()...});}, chunks);
            for (std::size_t i = computedCount; i < endCount; i++) {
                value = std::apply([this, value, i](auto &... s){return op(value, s->getElement(i)...);}, chunks);
                dst[i] = value;
            }

            return endCount;
        });
    }

private:
    OperatorType op;

    ElementType initialValue;
    std::tuple<ArgTypes...> args;
};

}

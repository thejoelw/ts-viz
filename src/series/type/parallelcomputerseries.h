#pragma once

#include "series/dataseries.h"
#include "program/progobj.h"

namespace {

template <typename ElementType, std::size_t size>
series::ChunkPtr<ElementType, size> extractSlice(series::DataSeries<ElementType, size> &x, std::size_t chunkIndex) {
    return x.getChunk(chunkIndex);
}

template <typename ElementType, std::size_t size>
std::vector<series::ChunkPtr<ElementType, size>> extractSlice(const program::ProgObjArray<series::DataSeries<ElementType, size> *> &x, std::size_t chunkIndex) {
    std::vector<series::ChunkPtr<ElementType, size>> res;
    res.reserve(x.getArr().size());
    for (series::DataSeries<ElementType, size> *ds : x.getArr()) {
        res.emplace_back(ds->getChunk(chunkIndex));
    }
    return std::move(res);
}

}

namespace series {

template <typename ElementType, typename OperatorType, typename... ArgTypes>
class ParallelComputerSeries : public DataSeries<ElementType> {
public:
    ParallelComputerSeries(app::AppContext &context, OperatorType op, ArgTypes &... args)
        : DataSeries<ElementType>(context)
        , op(op)
        , args(args...)
    {}

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        auto slices = std::apply([chunkIndex](auto &... x){return std::make_tuple(extractSlice(x, chunkIndex)...);}, args);
        return this->constructChunk([this, slices = std::move(slices)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            return std::apply([this, dst, computedCount](auto &... s){return op(dst, computedCount, s...);}, slices);
        });
    }

private:
    OperatorType op;

    std::tuple<ArgTypes...> args;
};

}

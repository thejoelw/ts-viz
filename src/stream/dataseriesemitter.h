#pragma once

#include "stream/seriesemitter.h"
#include "series/dataseries.h"

namespace stream {

template <typename ElementType>
class DataSeriesEmitter : public SeriesEmitter {
public:
    DataSeriesEmitter(const std::string &key, series::DataSeries<ElementType> *data)
        : SeriesEmitter(key)
        , data(data)
        , chunkIndex(static_cast<std::size_t>(-1))
        , curChunk(series::ChunkPtr<ElementType>::null())
        , nextChunk(series::ChunkPtr<ElementType>::null())
    {}

    std::pair<bool, double> getValue(std::size_t index);

private:
    series::DataSeries<ElementType> *data;

    std::size_t chunkIndex;
    series::ChunkPtr<ElementType> curChunk;
    series::ChunkPtr<ElementType> nextChunk; // We store the next chunk to so we don't gc/delete anything that we'll need
};

}

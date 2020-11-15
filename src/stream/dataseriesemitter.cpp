#include "dataseriesemitter.h"

namespace stream {

template <typename ElementType>
std::pair<bool, double> DataSeriesEmitter<ElementType>::getValue(std::size_t index) {
    typedef typename series::DataSeries<ElementType>::ChunkPtr ChunkPtr;

    ChunkPtr chunk = data->getChunk(index / CHUNK_SIZE);
    if (chunk->isDone()) {
        return std::pair<bool, double>(true, chunk->getData()[index % CHUNK_SIZE]);
    } else {
        return std::pair<bool, double>(false, NAN);
    }
}

template class DataSeriesEmitter<float>;
template class DataSeriesEmitter<double>;

}

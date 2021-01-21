#include "dataseriesemitter.h"

namespace stream {

template <typename ElementType>
std::pair<bool, double> DataSeriesEmitter<ElementType>::getValue(std::size_t index) {
    std::size_t ci = index / CHUNK_SIZE;
    if (ci != chunkIndex) {
        chunkIndex = ci;
        curChunk = data->getChunk(chunkIndex);
        nextChunk = data->getChunk(chunkIndex + 1);
    }

    if (index % CHUNK_SIZE < curChunk->getComputedCount()) {
        return std::pair<bool, double>(true, curChunk->getElement(index % CHUNK_SIZE));
    } else {
        return std::pair<bool, double>(false, NAN);
    }
}

template class DataSeriesEmitter<float>;
template class DataSeriesEmitter<double>;

}

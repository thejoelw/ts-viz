#include "dataseriesmetric.h"

namespace stream {

template <typename ElementType>
SeriesMetric::ValuePoller *DataSeriesMetric<ElementType>::makePoller(std::size_t index) {
    return pollers.alloc(data->getChunk(index / CHUNK_SIZE), index % CHUNK_SIZE);
}

template <typename ElementType>
void DataSeriesMetric<ElementType>::releasePoller(ValuePoller *poller) {
    pollers.free(static_cast<Poller *>(poller));
}

template <typename ElementType>
std::pair<bool, double> DataSeriesMetric<ElementType>::Poller::get() {
    if (elIdx < chunk->getComputedCount()) {
        return std::pair<bool, double>(true, chunk->getElement(elIdx));
    } else {
        return std::pair<bool, double>(false, NAN);
    }
}

template class DataSeriesMetric<float>;
template class DataSeriesMetric<double>;

}

#include "dataseries.h"

namespace series {

template <>
thread_local DataSeries<float>::Chunk *DataSeries<float>::activeComputingChunk = 0;

template <>
thread_local DataSeries<double>::Chunk *DataSeries<double>::activeComputingChunk = 0;

}

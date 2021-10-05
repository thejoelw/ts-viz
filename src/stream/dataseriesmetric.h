#pragma once

#include "stream/seriesmetric.h"
#include "series/dataseries.h"
#include "jw_util/pool.h"

namespace stream {

template <typename ElementType>
class DataSeriesMetric : public SeriesMetric {
public:
    DataSeriesMetric(const std::string &key, series::DataSeries<ElementType> *data)
        : SeriesMetric(key)
        , data(data)
    {}

    ValuePoller *makePoller(std::size_t index);
    void releasePoller(ValuePoller *poller);

private:
    class Poller : public ValuePoller {
    public:
        Poller(series::ChunkPtr<ElementType> &&chunk, std::size_t elIdx)
            : chunk(std::move(chunk))
            , elIdx(elIdx)
        {}

        std::pair<bool, double> get();

    private:
        series::ChunkPtr<ElementType> chunk;
        std::size_t elIdx;
    };

    series::DataSeries<ElementType> *data;

    inline static jw_util::Pool<Poller> pollers;
};

}

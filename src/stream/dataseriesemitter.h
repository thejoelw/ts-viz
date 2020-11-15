#pragma once

#include "stream/seriesemitter.h"
#include "series/dataseries.h"

namespace stream {

template <typename ElementType>
class DataSeriesEmitter : public SeriesEmitter {
public:
    DataSeriesEmitter(app::AppContext &context, const std::string &key, series::DataSeries<ElementType> *data)
        : SeriesEmitter(context, key)
        , data(data)
    {}

    std::pair<bool, double> getValue(std::size_t index);

private:
    series::DataSeries<ElementType> *data;
};

}

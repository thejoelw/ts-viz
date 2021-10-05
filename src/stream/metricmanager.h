#pragma once

#include <queue>

#include "app/tickercontext.h"
#include "stream/seriesmetric.h"

namespace stream {

class MetricManager : public app::TickerContext::TickableBase<MetricManager> {
public:
    MetricManager(app::AppContext &context);
    ~MetricManager();

    void addMetric(SeriesMetric *metric);
    void submitMetrics();

    void tick(app::TickerContext &tickerContext);

    bool isRunning() const;

private:
    std::vector<SeriesMetric *> curMetrics;
    std::queue<std::vector<std::pair<SeriesMetric *, SeriesMetric::ValuePoller *>>> metricQueue;
};

}

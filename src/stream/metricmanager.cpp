#include "metricmanager.h"

#include <iostream>
#include <cmath>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "app/options.h"

namespace stream {

MetricManager::MetricManager(app::AppContext &context)
    : TickableBase(context)
{}

MetricManager::~MetricManager() {
    assert(metricQueue.empty());
}

void MetricManager::addMetric(SeriesMetric *metric) {
    curMetrics.push_back(metric);
}

void MetricManager::submitMetrics() {
    for (std::size_t index : app::Options::getInstance().meterIndices) {
        std::vector<std::pair<SeriesMetric *, SeriesMetric::ValuePoller *>> rec;
        for (SeriesMetric *metric : curMetrics) {
            rec.emplace_back(metric, metric->makePoller(index));
        }
        metricQueue.emplace(std::move(rec));
    }
    curMetrics.clear();
}

void MetricManager::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    static thread_local rapidjson::StringBuffer buffer;
    static thread_local rapidjson::Writer<rapidjson::StringBuffer> writer;

    while (!metricQueue.empty()) {
        buffer.Clear();
        writer.Reset(buffer);

        writer.StartObject();

        for (std::pair<SeriesMetric *, SeriesMetric::ValuePoller *> metric : metricQueue.front()) {
            std::pair<bool, double> res = metric.second->get();
            if (!res.first) {
                goto finishLoop;
            }

            writer.Key(metric.first->getKey().data(), metric.first->getKey().size());
            if (std::isfinite(res.second)) {
                writer.Double(res.second);
            } else {
                writer.Null();
            }
        }

        writer.EndObject();

        std::cout << buffer.GetString() << std::endl;
        std::cout.flush();

        for (std::pair<SeriesMetric *, SeriesMetric::ValuePoller *> metric : metricQueue.front()) {
            metric.first->releasePoller(metric.second);
        }
        metricQueue.pop();
    }
    finishLoop:;
}

bool MetricManager::isRunning() const {
    return !metricQueue.empty();
}

}

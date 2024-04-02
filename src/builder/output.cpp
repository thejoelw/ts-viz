#include "program/resolver.h"
#include "stream/dataseriesemitter.h"
#include "stream/dataseriesmetric.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<float> *value) {
        return new stream::DataSeriesEmitter<float>(key, value);
    });

    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<double> *value) {
        return new stream::DataSeriesEmitter<double>(key, value);
    });

    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<float> *value, const std::string &hash) {
        (void) hash;
        return new stream::DataSeriesEmitter<float>(key, value);
    });

    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<double> *value, const std::string &hash) {
        (void) hash;
        return new stream::DataSeriesEmitter<double>(key, value);
    });

    resolver.decl("meter", [&context](const std::string &key, series::DataSeries<float> *value) {
        return new stream::DataSeriesMetric<float>(key, value);
    });

    resolver.decl("meter", [&context](const std::string &key, series::DataSeries<double> *value) {
        return new stream::DataSeriesMetric<double>(key, value);
    });
});

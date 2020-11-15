#include "program/resolver.h"
#include "stream/dataseriesemitter.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<float> *value) {
        return new stream::DataSeriesEmitter<float>(context, key, value);
    });

    resolver.decl("emit", [&context](const std::string &key, series::DataSeries<double> *value) {
        return new stream::DataSeriesEmitter<double>(context, key, value);
    });
});

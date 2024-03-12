#include "program/resolver.h"
#include "series/type/drawnseries.h"

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<float> *trigger){return new series::DrawnSeries<INPUT_SERIES_ELEMENT_TYPE, series::DataSeries<float>>(context, name, *trigger);});
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<double> *trigger){return new series::DrawnSeries<INPUT_SERIES_ELEMENT_TYPE, series::DataSeries<double>>(context, name, *trigger);});
});
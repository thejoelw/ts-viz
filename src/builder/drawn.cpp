#include "program/resolver.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"
#include "series/type/drawnseries.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<float> *trigger){return new series::DrawnSeries<INPUT_SERIES_ELEMENT_TYPE, series::DataSeries<float>>(context, name, *trigger);});
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<double> *trigger){return new series::DrawnSeries<INPUT_SERIES_ELEMENT_TYPE, series::DataSeries<double>>(context, name, *trigger);});
});

#else

#include "series/type/parallelopseries.h"

template <typename RealType> struct ReturnNan { RealType operator()(RealType a) const { return NAN; } };

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<float> *trigger){
        return new series::ParallelOpSeries<float, ReturnNan<float>, series::DataSeries<float>>(context, ReturnNan<float>(), *trigger);
    });
    resolver.decl("drawn", [&context](const std::string &name, series::DataSeries<double> *trigger){
        return new series::ParallelOpSeries<double, ReturnNan<double>, series::DataSeries<double>>(context, ReturnNan<double>(), *trigger);
    });
});

#endif

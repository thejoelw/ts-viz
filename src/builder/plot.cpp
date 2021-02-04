#include "program/resolver.h"

#include "render/dataseriesrenderer.h"

template <typename RealType>
void declPlot(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("plot", [&context](const std::string &name, series::DataSeries<RealType> *s, program::UncastNumber r, program::UncastNumber g, program::UncastNumber b, program::UncastNumber a) {
        return new render::DataSeriesRenderer<RealType>(context, name, s, r.value, g.value, b.value, a.value);
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declPlot<float>(context, resolver);
    declPlot<double>(context, resolver);
});

#include "program/resolver.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "render/dataseriesrenderer.h"
#endif

#include "series/invalidparameterexception.h"

template <typename RealType>
void declPlot(app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("plot", [&context](series::DataSeries<RealType> *s, const std::string &name, program::UncastNumber r, program::UncastNumber g, program::UncastNumber b, program::UncastNumber a) {
#if ENABLE_GRAPHICS
        render::DataSeriesRenderer<RealType> *res = new render::DataSeriesRenderer<RealType>(context, s, name);
        res->getDrawStyle().color[0] = r.value;
        res->getDrawStyle().color[1] = g.value;
        res->getDrawStyle().color[2] = b.value;
        res->getDrawStyle().color[3] = a.value;
        return res;
#else
        throw series::InvalidParameterException("plot command is not available when ENABLE_GRAPHICS is false");
        return std::monostate();
#endif
    });
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    declPlot<float>(context, resolver);
    declPlot<double>(context, resolver);
});

#include "program/resolver.h"
#include "render/dataseriesrenderer.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("plot", [&context](series::DataSeries<float> *s, const std::string &name, program::UncastNumber r, program::UncastNumber g, program::UncastNumber b, program::UncastNumber a){
        render::DataSeriesRenderer<float> *res = new render::DataSeriesRenderer<float>(context, s, name);
        res->getDrawStyle().color[0] = r.value;
        res->getDrawStyle().color[1] = g.value;
        res->getDrawStyle().color[2] = b.value;
        res->getDrawStyle().color[3] = a.value;
        return res;
    });

    resolver.decl("plot", [&context](series::DataSeries<double> *s, const std::string &name, program::UncastNumber r, program::UncastNumber g, program::UncastNumber b, program::UncastNumber a){
        render::DataSeriesRenderer<double> *res = new render::DataSeriesRenderer<double>(context, s, name);
        res->getDrawStyle().color[0] = r.value;
        res->getDrawStyle().color[1] = g.value;
        res->getDrawStyle().color[2] = b.value;
        res->getDrawStyle().color[3] = a.value;
        return res;
    });
});

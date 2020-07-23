#include "seriesrenderer.h"

#include "render/program/pointcloudprogram.h"
#include "render/imguirenderer.h"

namespace render {

SeriesRenderer::SeriesRenderer(app::AppContext &context)
    : TickableBase(context)
{
    graphics::GpuProgram::printExtensions(context);

    glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
    glDisable(GL_DEPTH_TEST);

//    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
//    glEnable(GL_PROGRAM_POINT_SIZE);
//    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glPointSize(2.0f);

    graphics::GL::catchErrors();

    context.get<PointCloudProgram>().make();

    context.get<ImguiRenderer>();
}

void SeriesRenderer::addSeries(const char *name, const series::Series<float> &s) {

}

void SeriesRenderer::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<ImguiRenderer::Ticker>();
}

void SeriesRenderer::tickClose(app::TickerContext &tickerContext) {
    glClear(GL_COLOR_BUFFER_BIT);
    context.get<PointCloudProgram>().draw();

//    glDrawRangeElements(GL_TRIANGLES, 0, sceneManager.getVertBuffer().getSize(), sceneManager.getFaceBuffer().getSize(), GL_UNSIGNED_INT, reinterpret_cast<void *>(0));
}

}

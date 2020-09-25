#include "renderer.h"

#include "render/program/linestripprogram.h"
#include "render/camera.h"

namespace render {

Renderer::Renderer(app::AppContext &context)
    : TickableBase(context)
{
    graphics::GpuProgram::printExtensions(context);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);

//    glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
//    glEnable(GL_PROGRAM_POINT_SIZE);
//    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glPointSize(2.0f);

    graphics::GL::catchErrors();
}

void Renderer::clearSeries() {
    added.clear();
}

void Renderer::addSeries(const char *name, series::Series *s) {
    added.push_back(s);
}

void Renderer::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<render::Camera::Ticker>();
}

void Renderer::tickClose(app::TickerContext &tickerContext) {
    glClear(GL_COLOR_BUFFER_BIT);

    std::size_t minX = std::max(0.0f, std::floorf(context.get<render::Camera>().getMin().x));
    std::size_t maxX = std::max(0.0f, std::ceilf(context.get<render::Camera>().getMax().x)) + 1;
    std::size_t stride = std::max(static_cast<std::size_t>(1), (maxX - minX) >> 12);

    for (series::Series *series : added) {
        series->draw(minX, maxX, stride);
    }
}

}

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "renderer.h"

#include "render/program/linestripprogram.h"
#include "render/camera.h"
#include "render/axes.h"

#include "defs/RENDER_RESOLUTION_X.h"

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

    context.get<Axes>();
}

void Renderer::clearSeries() {
    added.clear();
}

void Renderer::addSeries(SeriesRenderer *renderer) {
    added.push_back(renderer);
}

void Renderer::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<Camera::Ticker>();
}

void Renderer::tickClose(app::TickerContext &tickerContext) {
    (void) tickerContext;

    glClear(GL_COLOR_BUFFER_BIT);

    std::size_t minX = std::max(0.0f, std::floorf(context.get<render::Camera>().getMin().x));
    std::size_t maxX = std::max(0.0f, std::ceilf(context.get<render::Camera>().getMax().x)) + 1;
    std::size_t stride = std::max(static_cast<std::size_t>(1), (maxX - minX) / RENDER_RESOLUTION_X);

    for (SeriesRenderer *renderer : added) {
        renderer->draw(minX, maxX, stride);
    }
}

}

#endif

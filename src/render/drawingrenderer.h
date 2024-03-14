#pragma once

#include "render/seriesrenderer.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "graphics/glbuffer.h"
#include "graphics/type/element.h"
#include "render/program/linestripprogram.h"
#include "stream/drawingmanager.h"
#endif

namespace render {

class DrawingRenderer : public SeriesRenderer {
public:
    DrawingRenderer(app::AppContext &context, const std::string &name, bool enabled, float r, float g, float b, float a)
        : SeriesRenderer(context, name)
#if ENABLE_GRAPHICS
        , drawing(context.get<stream::DrawingManager>().getStream(name))
        , enabled(enabled)
        , remoteBuffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW)
        , drawStyle(r, g, b, a, true)
#endif
    {}

    void draw(std::size_t begin, std::size_t end, std::size_t stride);

    void updateTransform(float offset, float scale) {}

#if ENABLE_GRAPHICS
    typename LineStripProgram<double>::DrawStyle &getDrawStyle() {
        return drawStyle;
    }
#endif

private:
#if ENABLE_GRAPHICS
    stream::Drawing &drawing;

    bool enabled;

    graphics::GlVao vao;
    graphics::GlBuffer<double> remoteBuffer;

    typename LineStripProgram<double>::DrawStyle drawStyle;

    void updateDrawStyle();
#endif
};

}

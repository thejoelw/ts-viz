#pragma once

#include "render/seriesrenderer.h"
#include "stream/drawingmanager.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "graphics/glbuffer.h"
#include "graphics/type/element.h"
#include "render/program/linestripprogram.h"
#endif

namespace render {

class DrawingRenderer : public SeriesRenderer {
public:
    DrawingRenderer(app::AppContext &context, const std::string &name, stream::Drawing &drawing, bool enabled, float r, float g, float b, float a)
        : SeriesRenderer(context, name)
        , drawing(drawing)
#if ENABLE_GRAPHICS
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
    stream::Drawing &drawing;

#if ENABLE_GRAPHICS
    bool enabled;

    graphics::GlVao vao;
    graphics::GlBuffer<double> remoteBuffer;

    typename LineStripProgram<double>::DrawStyle drawStyle;

    void updateDrawStyle();
#endif
};

}

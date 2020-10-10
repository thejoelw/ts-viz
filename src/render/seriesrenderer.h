#pragma once

#include "graphics/glbuffer.h"
#include "graphics/type/element.h"
#include "render/program/linestripprogram.h"

namespace render {

template <typename ElementType>
class SeriesRenderer {
public:
    SeriesRenderer(app::AppContext &context)
        : context(context)
        , remoteBuffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW)
    {}

    void draw(std::size_t begin, std::size_t stride, const std::vector<ElementType> &data);

private:
    app::AppContext &context;

    graphics::GlVao vao;
    graphics::GlBuffer<ElementType> remoteBuffer;

    typename LineStripProgram<ElementType>::DrawStyle drawStyle;

    void updateDrawStyle();
};

}

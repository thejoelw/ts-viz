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

    void draw(std::size_t begin, std::size_t stride, const std::vector<ElementType> &data) {
        vao.bind();

        LineStripProgram<ElementType> &program = context.get<LineStripProgram<ElementType>>();

        remoteBuffer.bind();
        if (remoteBuffer.needs_resize(data.size())) {
            remoteBuffer.update_size(data.size());

            vao.assertBound();
            remoteBuffer.bind();

            program.make();
        }
        remoteBuffer.write(0, data.size(), data.data());

        program.draw(begin, stride, 0, data.size());

        vao.unbind();
    }

private:
    app::AppContext &context;

    graphics::GlVao vao;
    graphics::GlBuffer<ElementType> remoteBuffer;
};

}

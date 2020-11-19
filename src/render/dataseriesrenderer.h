#pragma once

#include "render/seriesrenderer.h"
#include "graphics/glbuffer.h"
#include "graphics/type/element.h"
#include "render/program/linestripprogram.h"
#include "series/base/dataseries.h"

namespace render {

template <typename ElementType>
class DataSeriesRenderer : public SeriesRenderer {
public:
    DataSeriesRenderer(app::AppContext &context, series::DataSeries<ElementType> *data, const std::string &name)
        : SeriesRenderer(context, name)
        , data(data)
        , remoteBuffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW)
    {}

    void draw(std::size_t begin, std::size_t end, std::size_t stride);

    typename LineStripProgram<ElementType>::DrawStyle &getDrawStyle() {
        return drawStyle;
    }

private:
    series::DataSeries<ElementType> *data;

    graphics::GlVao vao;
    graphics::GlBuffer<ElementType> remoteBuffer;

    typename LineStripProgram<ElementType>::DrawStyle drawStyle;

    struct Actions {
        bool fitX = false;
        bool fitY = false;
    };
    Actions updateDrawStyle();
};

}

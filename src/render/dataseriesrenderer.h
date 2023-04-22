#pragma once

#include <unordered_map>

#include "render/seriesrenderer.h"
#include "series/dataseries.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "graphics/glbuffer.h"
#include "graphics/type/element.h"
#include "render/program/linestripprogram.h"
#endif

namespace render {

template <typename ElementType>
class DataSeriesRenderer : public SeriesRenderer {
private:
    struct SelectionRegistry {
        SelectionRegistry(app::AppContext &context) {}

        std::unordered_map<std::string, bool> registry;
    };

public:
    DataSeriesRenderer(app::AppContext &context, const std::string &name, series::DataSeries<ElementType> *data, bool offset, bool enabled, float r, float g, float b, float a)
        : SeriesRenderer(context, name)
        , data(data)
#if ENABLE_GRAPHICS
        , enabled(enabled)
        , originalOffset(offset)
        , remoteBuffer(GL_ARRAY_BUFFER, GL_STREAM_DRAW)
        , drawStyle(r, g, b, a, true)
#endif
    {
        std::size_t pos = name.find('#');
        if (pos == std::string::npos) {
            tag = name;
        } else {
            tag = name.substr(pos + 1);
        }
    }

    void draw(std::size_t begin, std::size_t end, std::size_t stride);

    void updateTransform(float offset, float scale);

#if ENABLE_GRAPHICS
    typename LineStripProgram<ElementType>::DrawStyle &getDrawStyle() {
        return drawStyle;
    }
#endif

private:
    series::DataSeries<ElementType> *data;

#if ENABLE_GRAPHICS
    bool enabled;

    std::string tag;

    bool originalOffset;
    float offset = 0.0f;
    float scale = 1.0f;

    graphics::GlVao vao;
    graphics::GlBuffer<ElementType> remoteBuffer;

    typename LineStripProgram<ElementType>::DrawStyle drawStyle;

    struct Actions {
        bool fitX = false;
        bool fitY = false;
    };
    Actions updateDrawStyle();

    bool &isSelected() const;
#endif
};

}

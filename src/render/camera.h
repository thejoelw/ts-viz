#pragma once

#include "graphics/glm.h"
#include "glm/vec2.hpp"

#include "app/tickercontext.h"
#include "graphics/glbuffer.h"
#include "graphics/glvao.h"
#include "graphics/type/point.h"

namespace render {

class Camera : public app::TickerContext::TickableBase<Camera, app::TickerContext::ScopedCaller<Camera>> {
public:
    enum class MouseRegion {
        None,
        Left, Right,
        Top, Bottom,
    };

    Camera(app::AppContext &context);

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);

    glm::vec2 getMin() const { return min; }
    glm::vec2 getMax() const { return max; }

    glm::vec2 getOffset() const { return offset; }
    glm::vec2 getScale() const { return scale; }

    void setBoundsX(float minX, float maxX) {
        min.x = minX;
        max.x = maxX;
    }
    void setBoundsY(float minY, float maxY) {
        min.y = minY;
        max.y = maxY;
    }

private:
    static constexpr float mouseRegionSize = 0.2f;

    glm::vec2 mousePosition;
    MouseRegion mouseRegion;

    glm::vec2 min;
    glm::vec2 max;

    glm::vec2 offset;
    glm::vec2 scale;

    graphics::GlVao vao;
    graphics::GlBuffer<graphics::Point> remoteBuffer;

    glm::vec2 computeDelta() const;
    MouseRegion computeMouseRegion() const;
    void drawMouseRegion();
};

}

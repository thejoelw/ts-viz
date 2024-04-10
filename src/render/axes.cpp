#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "axes.h"

#include <iomanip>
#include <sstream>

#include "graphics/imgui.h"
#include "render/imguirenderer.h"
#include "render/camera.h"
#include "app/window.h"

namespace render {

Axes::Axes(app::AppContext &context)
    : TickableBase(context)
{}

void Axes::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<Camera::Ticker>();
}

void Axes::tickClose(app::TickerContext &tickerContext) {
    if (!tickerContext.getAppContext().get<app::Window>().shouldRender()) {
        return;
    }

    static constexpr std::uint32_t axisColor = 0x44FFFFFF;
    static constexpr std::uint32_t textColor = 0x88FFFFFF;

    app::Window::Dimensions dims = context.get<app::Window>().dimensions;
    dims.width /= 2.0;
    dims.height /= 2.0;

    Camera &camera = context.get<Camera>();

    glm::vec2 min = camera.getMin();
    glm::vec2 max = camera.getMax();
    glm::vec2 range = max - min;
    glm::vec2 ts(getBestTickSize(range.x / (dims.width / 128.0f)), getBestTickSize(range.y / (dims.height / 128.0f)));
    glm::vec2 tb(std::floorf(min.x / ts.x) * ts.x, std::floorf(min.y / ts.y) * ts.y);
    glm::vec2 te(std::ceilf(max.x / ts.x) * ts.x, std::ceilf(max.y / ts.y) * ts.y);

    for (float x = tb.x; x < te.x; x += ts.x) {
        std::stringstream stream;
        stream << x;
        std::string label = stream.str();

        float vx = (x - min.x) / range.x * dims.width;
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(vx, 0.0f), ImVec2(vx, dims.height - 16.0f), axisColor);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(vx - label.size() * 3.0f, dims.height - 16.0f), textColor, label.data());
    }

    for (float y = tb.y; y < te.y; y += ts.y) {
        std::stringstream stream;
        stream << y;
        std::string label = stream.str();

        float vy = (1.0f - (y - min.y) / range.y) * dims.height;
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(label.size() * 7.0f + 2.0f, vy), ImVec2(dims.width, vy), axisColor);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(0.0f, vy - 6.0f), textColor, label.data());
    }
}

float Axes::getBestTickSize(float size) {
    float p10 = std::powf(10.0f, std::floorf(std::log10f(size)));

    float p50 = p10 * 5.0f;
    if (p50 < size) { return p50; }

    float p20 = p10 * 2.0f;
    if (p20 < size) { return p20; }

    return p10;
}

}

#endif

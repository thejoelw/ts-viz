#include "axes.h"

#include "graphics/imgui.h"
#include "render/imguirenderer.h"
#include "render/camera.h"

namespace render {

Axes::Axes(app::AppContext &context)
    : TickableBase(context)
{}

void Axes::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<Camera::Ticker>();
}

void Axes::tickClose(app::TickerContext &tickerContext) {
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(100, 100), ImVec2(200, 300), 0xFFFFFF88);
    ImGui::GetForegroundDrawList()->AddText(ImVec2(100, 100), 0xFFFFFFFF, "abc");
}

}

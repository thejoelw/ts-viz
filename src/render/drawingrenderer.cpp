#include "drawingrenderer.h"

#include <cassert>

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "graphics/imgui.h"
#include "render/camera.h"
#endif

namespace render {

void DrawingRenderer::draw(std::size_t begin, std::size_t end, std::size_t stride) {
#if ENABLE_GRAPHICS
    assert(begin <= end);

    updateDrawStyle();

    if (!enabled) {
        return;
    }

    static thread_local std::vector<double> sample;
    sample.clear();

    for (std::size_t i = begin; i < end; i += stride) {
        sample.push_back(drawing.sample(i));
    }

    vao.bind();

    LineStripProgram<double> &program = context.get<LineStripProgram<double>>();

    remoteBuffer.bind();
    if (remoteBuffer.needs_resize(sample.size())) {
        remoteBuffer.update_size(sample.size());

        vao.assertBound();
        remoteBuffer.bind();

        program.make();
    }
    remoteBuffer.write(0, sample.size(), sample.data());

    program.draw(begin, stride, 0, sample.size(), drawStyle);

    vao.unbind();
#else
    (void) begin;
    (void) end;
    (void) stride;

    assert(false);
#endif
}

#if ENABLE_GRAPHICS
void DrawingRenderer::updateDrawStyle() {
    if (ImGui::Begin("Series")) {
        double x = context.get<Camera>().getMousePos().x;
        double y = drawing.sample(x);

        std::string uuid = std::to_string(reinterpret_cast<std::uintptr_t>(this));

        ImGui::Checkbox(("##enabled-" + uuid).data(), &enabled);

        ImGui::SameLine(32);
        ImGui::ColorEdit4(("color##color-" + uuid).data(), drawStyle.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

        ImGui::SameLine(56);
        ImGui::Text("%s", name.data());

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 116);
        ImGui::Button(("y:" + std::to_string(y) + "##y-" + uuid).data(), ImVec2(90, 0));

        ImGui::SameLine();
        ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = ImVec4(0.40f, 0.55f, 0.75f, 0.54f);

        bool isSelected = false;
        ImGui::Checkbox(("selected##selected-" + uuid).data(), &isSelected);
    }
    ImGui::End();
}

#endif

}

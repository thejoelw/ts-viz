#include "seriesrenderer.h"

#include "graphics/imgui.h"

namespace render {

template <typename ElementType>
void SeriesRenderer<ElementType>::draw(std::size_t begin, std::size_t stride, const std::vector<ElementType> &data) {
    updateDrawStyle();

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

    program.draw(begin, stride, 0, data.size(), drawStyle);

    vao.unbind();
}

template <typename ElementType>
void SeriesRenderer<ElementType>::updateDrawStyle() {
    if (ImGui::Begin("Series")) {
        std::string uuid = std::to_string(reinterpret_cast<std::uintptr_t>(this));
        ImGui::ColorEdit4(("color##" + uuid).data(), drawStyle.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine(35);
        drawStyle.smooth &= !ImGui::Button(("normal##" + uuid).data());
        ImGui::SameLine(90);
        drawStyle.smooth |= ImGui::Button(("bold##" + uuid).data());
    }
    ImGui::End();
}

template class SeriesRenderer<float>;
template class SeriesRenderer<double>;

}

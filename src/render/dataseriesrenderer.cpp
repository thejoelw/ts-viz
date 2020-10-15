#include "dataseriesrenderer.h"

#include "graphics/imgui.h"
#include "render/camera.h"

namespace render {

template <typename ElementType>
void DataSeriesRenderer<ElementType>::draw(std::size_t begin, std::size_t end, std::size_t stride) {
    assert(begin <= end);

    static thread_local std::vector<ElementType> sample;
    sample.clear();

    for (std::size_t i = begin; i < end; i += stride) {
        typedef typename series::DataSeries<ElementType>::Chunk Chunk;
        Chunk *chunk = data->getChunk(i / Chunk::size);
        if (chunk->isDone()) {
            sample.push_back(chunk->getData()[i % Chunk::size]);
        } else {
            sample.push_back(NAN);
        }
    }

    Actions actions = updateDrawStyle();
    if (actions.fitY) {
        ElementType min = std::numeric_limits<float>::infinity();
        ElementType max = -std::numeric_limits<float>::infinity();

        for (ElementType val : sample) {
            if (!isnan(val)) {
                if (val < min) { min = val; }
                if (val > max) { max = val; }
            }
        }

        ElementType center = (min + max) * 0.5f;
        min += (min - center) * 0.1f;
        max += (max - center) * 0.1f;
        if (min > center - 1e-9) {
            min = center - 1e-9;
        }
        if (max < center + 1e-9) {
            max = center + 1e-9;
        }

        assert(min != max);

        if (min != std::numeric_limits<float>::infinity()) {
            context.get<render::Camera>().getMin().y = min;
        }
        if (max != -std::numeric_limits<float>::infinity()) {
            context.get<render::Camera>().getMax().y = max;
        }
    }

    vao.bind();

    LineStripProgram<ElementType> &program = context.get<LineStripProgram<ElementType>>();

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
}

template <typename ElementType>
typename DataSeriesRenderer<ElementType>::Actions DataSeriesRenderer<ElementType>::updateDrawStyle() {
    Actions actions;

    if (ImGui::Begin("Series")) {
        std::string uuid = std::to_string(reinterpret_cast<std::uintptr_t>(this));
        ImGui::Text("%s", name.data());
        ImGui::ColorEdit4(("color##" + uuid).data(), drawStyle.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine(35);
        drawStyle.smooth &= !ImGui::Button(("normal##" + uuid).data());
        ImGui::SameLine(90);
        drawStyle.smooth |= ImGui::Button(("bold##" + uuid).data());
        ImGui::SameLine(145);
        if (ImGui::Button(("fit-y##" + uuid).data())) {
            actions.fitY = true;
        }
    }
    ImGui::End();

    return actions;
}

template class DataSeriesRenderer<float>;
template class DataSeriesRenderer<double>;

}

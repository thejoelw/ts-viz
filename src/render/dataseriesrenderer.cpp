#include "defs/ENABLE_GRAPHICS.h"

#include "dataseriesrenderer.h"

#if ENABLE_GRAPHICS
#include "graphics/imgui.h"
#include "render/camera.h"
#endif

namespace render {

template <typename ElementType>
void DataSeriesRenderer<ElementType>::draw(std::size_t begin, std::size_t end, std::size_t stride) {
#if ENABLE_GRAPHICS
    assert(begin <= end);

    static thread_local std::vector<ElementType> sample;
    sample.clear();

    for (std::size_t i = begin; i < end; i += stride) {
        series::ChunkPtr<ElementType> chunk = data->getChunk(i / CHUNK_SIZE);
        if (i % CHUNK_SIZE < chunk->getComputedCount()) {
            sample.push_back(chunk->getElement(i % CHUNK_SIZE));
        } else {
            sample.push_back(NAN);
        }
    }

    Actions actions = updateDrawStyle();
    if (actions.fitY) {
        ElementType min = std::numeric_limits<float>::infinity();
        ElementType max = -std::numeric_limits<float>::infinity();

        for (ElementType val : sample) {
            if (!std::isnan(val)) {
                if (val < min) { min = val; }
                if (val > max) { max = val; }
            }
        }

        if (min != std::numeric_limits<float>::infinity() && max != -std::numeric_limits<float>::infinity()) {
            ElementType center = (min + max) * 0.5f;
            min += (min - center) * 0.1f;
            max += (max - center) * 0.1f;
            if (min > center - 1e-9) {
                min = center - 1e-9;
            }
            if (max < center + 1e-9) {
                max = center + 1e-9;
            }

            assert(min < max);

            context.get<render::Camera>().getMin().y = min;
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
#else
    (void) begin;
    (void) end;
    (void) stride;

    assert(false);
#endif
}

#if ENABLE_GRAPHICS
template <typename ElementType>
typename DataSeriesRenderer<ElementType>::Actions DataSeriesRenderer<ElementType>::updateDrawStyle() {
    Actions actions;

    if (ImGui::Begin("Series")) {
        float x = context.get<Camera>().getMousePos().x;
        ElementType y;
        bool hasY = false;
        if (x >= 0.0f) {
            std::size_t ix = x;
            series::ChunkPtr<ElementType> chunk0 = data->getChunk(ix / CHUNK_SIZE);
            series::ChunkPtr<ElementType> chunk1 = data->getChunk((ix + 1) / CHUNK_SIZE);
            if (ix % CHUNK_SIZE < chunk0->getComputedCount() && (ix + 1) % CHUNK_SIZE < chunk1->getComputedCount()) {
                ElementType y0 = chunk0->getElement(ix % CHUNK_SIZE);
                ElementType y1 = chunk1->getElement((ix + 1) % CHUNK_SIZE);
                y = y0 * ((ix + 1) - x) + y1 * (x - ix);
                hasY = true;
            }
        }

        std::string uuid = std::to_string(reinterpret_cast<std::uintptr_t>(this));
        ImGui::ColorEdit4(("color##" + uuid).data(), drawStyle.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine(32);
        ImGui::Text("%s", name.data());
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 90);
        if (ImGui::Button(("y:" + (hasY ? std::to_string(y) : "?") + "##" + uuid).data(), ImVec2(90, 0))) {
            actions.fitY = true;
        }
    }
    ImGui::End();

    return actions;
}
#endif

template class DataSeriesRenderer<float>;
template class DataSeriesRenderer<double>;

}

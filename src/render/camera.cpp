#include "camera.h"

#include "graphics/imgui.h"
#include "render/imguirenderer.h"
#include "app/window.h"
#include "render/program/fillprogram.h"

namespace render {

Camera::Camera(app::AppContext &context)
    : TickableBase(context)
    , min(0.0f, 0.0f)
    , max(100.0f, 10.0f)
    , offset(0.0f)
    , scale(0.01f)
    , remoteBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW)
{}

void Camera::tickOpen(app::TickerContext &tickerContext) {
    tickerContext.get<ImguiRenderer::Ticker>();

    glm::vec2 prevMousePos = mousePosition;

    app::Window &window = context.get<app::Window>();
    app::Window::MousePosition mp = window.getMousePosition();
    mousePosition.x = 4.0 * mp.x / window.dimensions.width - 1.0;
    mousePosition.y = 4.0 * mp.y / window.dimensions.height - 1.0;

    glm::vec2 delta = computeDelta();
    mouseRegion = computeMouseRegion();

    switch (mouseRegion) {
        case MouseRegion::None: min += delta; max += delta; break;
        case MouseRegion::Left: min += delta; max.y += delta.y; break;
        case MouseRegion::Right: min.y += delta.y; max += delta; break;
        case MouseRegion::Top: min += delta; max.x += delta.x; break;
        case MouseRegion::Bottom: min.x += delta.x; max += delta; break;
    }

    if (window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
        glm::vec2 delta = mousePosition - prevMousePos;
        delta *= (max - min) * glm::vec2(-0.5f, 0.5f);
        max += delta;
        min += delta;
    }

    scale = 2.0f / (max - min);
    offset = -1.0f - scale * min;

    if (ImGui::Begin("Mouse")) {
        glm::vec2 pos = min + (mousePosition + glm::vec2(1.0f, -1.0f)) * (max - min) * glm::vec2(0.5f, -0.5f);
        ImGui::Text("(%f, %f)", pos.x, pos.y);
    }
    ImGui::End();
}

void Camera::tickClose(app::TickerContext &tickerContext) {
    static constexpr std::uint32_t axisColor = 0x88000000;

    drawMouseRegion();

    if (!ImGui::GetIO().WantCaptureMouse) {
        app::Window &window = context.get<app::Window>();
        float vx = (mousePosition.x + 1.0f) * window.dimensions.width * 0.25f;
        float vy = (mousePosition.y + 1.0f) * window.dimensions.height * 0.25f;
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(vx, 0.0f), ImVec2(vx, window.dimensions.height), axisColor);
        ImGui::GetForegroundDrawList()->AddLine(ImVec2(0.0f, vy), ImVec2(window.dimensions.width, vy), axisColor);
    }
}

glm::vec2 Camera::computeDelta() const {
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return glm::vec2(0.0f, 0.0f);
    }

    app::Window &window = context.get<app::Window>();

    glm::vec2 delta;
    delta.x = static_cast<float>(window.isKeyPressed(GLFW_KEY_RIGHT)) - static_cast<float>(window.isKeyPressed(GLFW_KEY_LEFT));
    delta.y = static_cast<float>(window.isKeyPressed(GLFW_KEY_UP)) - static_cast<float>(window.isKeyPressed(GLFW_KEY_DOWN));
    delta *= 0.02f;
    delta *= max - min;

    return delta;
}

Camera::MouseRegion Camera::computeMouseRegion() const {
    if (ImGui::GetIO().WantCaptureMouse) {
        return MouseRegion::None;
    }

    float dirs[5] = {
        1.0f - mouseRegionSize,
        -mousePosition.x,
        mousePosition.x,
        -mousePosition.y,
        mousePosition.y,
    };
    switch (std::max_element(dirs, dirs + 5) - dirs) {
        case 1: return MouseRegion::Left;
        case 2: return MouseRegion::Right;
        case 3: return MouseRegion::Bottom;
        case 4: return MouseRegion::Top;
        default: return MouseRegion::None;
    }
}

void Camera::drawMouseRegion() {
    FillProgram &program = context.get<FillProgram>();

    static constexpr std::size_t size = 6;
    static constexpr float mrp = 1.0f - mouseRegionSize;
    static thread_local graphics::Point leftPts[size] = {
        {-1.0f, -1.0f}, {-mrp, -mrp}, {-mrp, mrp},
        {-mrp, mrp}, {-1.0f, 1.0f}, {-1.0f, -1.0f},
    };
    static thread_local graphics::Point rightPts[size] = {
        {1.0f, -1.0f}, {mrp, -mrp}, {mrp, mrp},
        {mrp, mrp}, {1.0f, 1.0f}, {1.0f, -1.0f},
    };
    static thread_local graphics::Point topPts[size] = {
        {1.0f, -1.0f}, {mrp, -mrp}, {-mrp, -mrp},
        {-mrp, -mrp}, {-1.0f, -1.0f}, {1.0f, -1.0f},
    };
    static thread_local graphics::Point bottomPts[size] = {
        {1.0f, 1.0f}, {mrp, mrp}, {-mrp, mrp},
        {-mrp, mrp}, {-1.0f, 1.0f}, {1.0f, 1.0f},
    };

    graphics::Point *pts;

    switch (mouseRegion) {
        case MouseRegion::None: return;
        case MouseRegion::Left: pts = leftPts; break;
        case MouseRegion::Right: pts = rightPts; break;
        case MouseRegion::Top: pts = topPts; break;
        case MouseRegion::Bottom: pts = bottomPts; break;
    }

    vao.bind();

    static graphics::Point *lastPts = 0;
    if (pts != lastPts) {
        remoteBuffer.bind();
        if (remoteBuffer.needs_resize(size)) {
            remoteBuffer.update_size(size);

            vao.assertBound();
            remoteBuffer.bind();

            program.make();
        }
        remoteBuffer.write(0, size, pts);

        lastPts = pts;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    graphics::GL::catchErrors();

    program.draw(0, size, glm::vec4(0.0f, 0.0f, 0.0f, 0.2f));

    glDisable(GL_BLEND);

    vao.unbind();
}

}

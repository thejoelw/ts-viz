#include "linestripprogram.h"

#include "spdlog/logger.h"

#include "graphics/type/element.h"
#include "render/shaders.h"
#include "render/camera.h"

namespace {

bool prevStyleSmooth = false;

}

namespace render {

template <typename ElementType>
LineStripProgram<ElementType>::LineStripProgram(app::AppContext &context)
    : Program(context)
{}

template <typename ElementType>
void LineStripProgram<ElementType>::insertDefines() {
    Program::insertDefines();

    insertElementTypeDef();
    graphics::Element<ElementType>::insertDefines(defines);
}

template <>
void LineStripProgram<float>::insertElementTypeDef() {
    defines.set("ELEMENT_TYPE", std::string("float"));
}
template <>
void LineStripProgram<double>::insertElementTypeDef() {
    defines.set("ELEMENT_TYPE", std::string("double"));
}

template <typename ElementType>
void LineStripProgram<ElementType>::setupProgram() {
    Program::setupProgram();

    context.get<spdlog::logger>().debug("Compiling main vertex shader");
    std::string vertShaderStr = std::string(Shaders::mainVert);
    attachShader(GL_VERTEX_SHADER, std::move(vertShaderStr), defines);

    context.get<spdlog::logger>().debug("Compiling main fragment shader");
    std::string fragShaderStr = std::string(Shaders::mainFrag);
    attachShader(GL_FRAGMENT_SHADER, std::move(fragShaderStr), defines);
}

template <typename ElementType>
void LineStripProgram<ElementType>::linkProgram() {
    Program::linkProgram();

    offsetLocation = glGetUniformLocation(getProgramId(), "offset");
    graphics::GL::catchErrors();

    scaleLocation = glGetUniformLocation(getProgramId(), "scale");
    graphics::GL::catchErrors();

    colorLocation = glGetUniformLocation(getProgramId(), "color");
    graphics::GL::catchErrors();
}

template <typename ElementType>
void LineStripProgram<ElementType>::draw(std::size_t begin, std::size_t stride, std::size_t offsetIndex, std::size_t count, DrawStyle style) {
    Program::bind();

    glm::vec2 offset = context.get<render::Camera>().getOffset();
    glm::vec2 scale = context.get<render::Camera>().getScale();

    offset.x += scale.x * static_cast<float>(begin);
    scale.x *= static_cast<float>(stride);

    glUniform2f(offsetLocation, offset.x, offset.y);
    glUniform2f(scaleLocation, scale.x, scale.y);
    glUniform4f(colorLocation, style.color[0], style.color[1], style.color[2], style.color[3]);
    graphics::GL::catchErrors();

    if (style.smooth != prevStyleSmooth) {
        if (style.smooth) {
            glEnable(GL_LINE_SMOOTH);
        } else {
            glDisable(GL_LINE_SMOOTH);
        }
        graphics::GL::catchErrors();

        prevStyleSmooth = style.smooth;
    }

    glDrawArrays(GL_LINE_STRIP, offsetIndex, count);
    graphics::GL::catchErrors();
}

template class LineStripProgram<float>;
template class LineStripProgram<double>;

}

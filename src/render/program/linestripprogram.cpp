#include "linestripprogram.h"

#include "spdlog/logger.h"

#include "graphics/type/element.h"
#include "render/shaders.h"
#include "render/camera.h"

namespace render {

template <typename ElementType>
LineStripProgram<ElementType>::LineStripProgram(app::AppContext &context)
    : Program(context)
{
    vao.bind();
    graphics::Element<ElementType>::setupVao(vao);
    vao.unbind();
}

template <typename ElementType>
void LineStripProgram<ElementType>::insertDefines(Defines &defines) {
    Program::insertDefines(defines);

    insertElementTypeDef(defines);
    vao.insertDefines(defines);
}

template <>
void LineStripProgram<float>::insertElementTypeDef(Defines &defines) {
    defines.set("ELEMENT_TYPE", std::string("float"));
}
template <>
void LineStripProgram<double>::insertElementTypeDef(Defines &defines) {
    defines.set("ELEMENT_TYPE", std::string("double"));
}

template <typename ElementType>
void LineStripProgram<ElementType>::setupProgram(const Defines &defines) {
    Program::setupProgram(defines);

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
void LineStripProgram<ElementType>::draw(std::size_t begin, std::size_t stride, std::size_t offsetIndex, std::size_t count) {
    Program::bind();

    assertLinked();
    vao.assertBound();

    glm::vec2 offset = context.get<render::Camera>().getOffset();
    glm::vec2 scale = context.get<render::Camera>().getScale();

    offset += scale * static_cast<float>(begin);
    scale *= stride;

    glUniform2f(offsetLocation, offset.x, offset.y);
    glUniform2f(scaleLocation, scale.x, scale.y);
    glUniform4f(colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);
    graphics::GL::catchErrors();

    glDrawArrays(GL_LINE_STRIP, offsetIndex, count);
    graphics::GL::catchErrors();
}

template class LineStripProgram<float>;
template class LineStripProgram<double>;

}

#include "fillprogram.h"

#include "spdlog/logger.h"

#include "graphics/type/point.h"
#include "render/shaders.h"
#include "render/camera.h"

namespace render {

FillProgram::FillProgram(app::AppContext &context)
    : Program(context)
{}

void FillProgram::insertDefines() {
    Program::insertDefines();

    graphics::Point::insertDefines(defines);
}

void FillProgram::setupProgram() {
    Program::setupProgram();

    context.get<spdlog::logger>().debug("Compiling fill vertex shader");
    std::string vertShaderStr = std::string(Shaders::fillVert);
    attachShader(GL_VERTEX_SHADER, std::move(vertShaderStr), defines);

    context.get<spdlog::logger>().debug("Compiling fill fragment shader");
    std::string fragShaderStr = std::string(Shaders::fillFrag);
    attachShader(GL_FRAGMENT_SHADER, std::move(fragShaderStr), defines);
}

void FillProgram::linkProgram() {
    Program::linkProgram();

    colorLocation = glGetUniformLocation(getProgramId(), "color");
    graphics::GL::catchErrors();
}

void FillProgram::draw(std::size_t offsetIndex, std::size_t count, glm::vec4 color) {
    Program::bind();

    glUniform4f(colorLocation, color.r, color.g, color.b, color.a);
    graphics::GL::catchErrors();

    glDrawArrays(GL_TRIANGLES, offsetIndex, count);
    graphics::GL::catchErrors();
}

}

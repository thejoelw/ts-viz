#include "pointcloudprogram.h"

#include "spdlog/logger.h"

#include "graphics/glbufferbase.h"
#include "render/shaders.h"
#include "graphics/imgui.h"

namespace render {

PointCloudProgram::PointCloudProgram(app::AppContext &context)
    : Program(context)
{}

void PointCloudProgram::insertDefines(Defines &defines) {
    Program::insertDefines(defines);
}

void PointCloudProgram::setupProgram(const Defines &defines) {
    Program::setupProgram(defines);

    context.get<spdlog::logger>().debug("Compiling point cloud vertex shader");
    std::string vertShaderStr = std::string(Shaders::mainVert);
    attachShader(GL_VERTEX_SHADER, std::move(vertShaderStr), defines);

    context.get<spdlog::logger>().debug("Compiling point cloud fragment shader");
    std::string fragShaderStr = std::string(Shaders::mainFrag);
    attachShader(GL_FRAGMENT_SHADER, std::move(fragShaderStr), defines);
}

void PointCloudProgram::linkProgram() {
    Program::linkProgram();

    offsetLocation = glGetUniformLocation(getProgramId(), "offset");
    graphics::GL::catchErrors();

    scaleLocation = glGetUniformLocation(getProgramId(), "scale");
    graphics::GL::catchErrors();
}

void PointCloudProgram::draw() {
    Program::bind();

    assertLinked();

    /*
    Vao &vao = context.get<Vao>();
    vao.bind();

    SceneManager &sceneManager = context.get<SceneManager>();
    sceneManager.getMeshBuffer().sync(vao);
    sceneManager.getMaterialBuffer().sync(vao);
    sceneManager.getPointBuffer().sync(vao);
*/

    if (ImGui::Begin("Debug")) {
//        if (ImGui::Button("Toggle Triangles")) {
//            showTrianglesValue = !showTrianglesValue;
//            showTrianglesDirty = true;
//        }
    }
    ImGui::End();

    /*
    if (eyePosLocation != -1) {
        glm::vec3 eyePos = context.get<render::Camera>().getEyePos();
        glUniform3f(eyePosLocation, eyePos.x, eyePos.y, eyePos.z);
    }

    if (eyeDirLocation != -1) {
        glm::vec3 eyeDir = context.get<render::Camera>().getEyeDir();
        glUniform3f(eyeDirLocation, eyeDir.x, eyeDir.y, eyeDir.z);
    }

    glDrawArrays(GL_POINTS, 0, sceneManager.getPointBuffer().getExtentSize());
    graphics::GL::catchErrors();

    vao.unbind();
    */
}

}

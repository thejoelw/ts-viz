#pragma once

#include "graphics/glm.h"
#include "glm/vec4.hpp"

#include "app/appcontext.h"
#include "graphics/glvao.h"
#include "render/program/program.h"

namespace render {

class FillProgram : public Program {
public:
    FillProgram(app::AppContext &context);

    virtual void insertDefines(Defines &defines);
    virtual void setupProgram(const Defines &defines);
    virtual void linkProgram();

    void draw(std::size_t offset, std::size_t count, glm::vec4 color);

    graphics::GlVao &getVao() {
        return vao;
    }

private:
    graphics::GlVao vao;

    GLint colorLocation;
};

}

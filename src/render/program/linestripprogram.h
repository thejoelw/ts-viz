#pragma once

#include "app/appcontext.h"
#include "graphics/glvao.h"
#include "render/program/program.h"

namespace render {

template <typename ElementType>
class LineStripProgram : public Program {
public:
    LineStripProgram(app::AppContext &context);

    virtual void insertDefines();
    virtual void setupProgram();
    virtual void linkProgram();

    struct DrawStyle {
        GLfloat color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        bool smooth = true;
    };

    void draw(std::size_t begin, std::size_t stride, std::size_t offsetIndex, std::size_t count, DrawStyle style);

private:
    GLint offsetLocation;
    GLint scaleLocation;
    GLint colorLocation;

    void insertElementTypeDef();
};

}

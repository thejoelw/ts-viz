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
        DrawStyle(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f, bool smooth = true)
            : color{r, g, b, a}
            , smooth(smooth)
        {}

        GLfloat color[4];
        bool smooth;
    };

    void draw(std::size_t begin, std::size_t stride, std::size_t offsetIndex, std::size_t count, DrawStyle style);

private:
    GLint offsetLocation;
    GLint scaleLocation;
    GLint colorLocation;

    void insertElementTypeDef();
};

}

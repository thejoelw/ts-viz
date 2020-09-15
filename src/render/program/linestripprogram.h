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

    void draw(std::size_t begin, std::size_t stride, std::size_t offsetIndex, std::size_t count);

private:
    GLint offsetLocation;
    GLint scaleLocation;
    GLint colorLocation;

    void insertElementTypeDef();
};

}

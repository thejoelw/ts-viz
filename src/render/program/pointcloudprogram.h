#pragma once

#include "app/appcontext.h"
#include "graphics/glvao.h"
#include "render/program/program.h"

namespace render {

class PointCloudProgram : public Program {
public:
    PointCloudProgram(app::AppContext &context);

    virtual void insertDefines(Defines &defines);
    virtual void setupProgram(const Defines &defines);
    virtual void linkProgram();

    virtual void draw();

private:
    GLint offsetLocation;
    GLint scaleLocation;
};

}

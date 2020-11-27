#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "point.h"

#include "graphics/glvao.h"

namespace graphics {

void Point::insertDefines(render::Program::Defines &defines) {
    GLuint positionLocation = defines.addProgramAttribute("POSITION_LOCATION", 1);
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void *>(offsetof(Point, position)));
    glEnableVertexAttribArray(positionLocation);
    graphics::GL::catchErrors();
}

}

#endif

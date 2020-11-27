#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "element.h"

#include "graphics/glvao.h"

namespace graphics {

template <>
void Element<float>::insertDefines(render::Program::Defines &defines) {
    GLuint positionLocation = defines.addProgramAttribute("POSITION_Y_LOCATION", 1);
    glVertexAttribPointer(positionLocation, 1, GL_FLOAT, GL_FALSE, sizeof(Element), reinterpret_cast<void *>(offsetof(Element, position)));
    glEnableVertexAttribArray(positionLocation);
    graphics::GL::catchErrors();
}

template <>
void Element<double>::insertDefines(render::Program::Defines &defines) {
    GLuint positionLocation = defines.addProgramAttribute("POSITION_Y_LOCATION", 1);
    glVertexAttribLPointer(positionLocation, 1, GL_DOUBLE, sizeof(Element), reinterpret_cast<void *>(offsetof(Element, position)));
    glEnableVertexAttribArray(positionLocation);
    graphics::GL::catchErrors();
}

}

#endif

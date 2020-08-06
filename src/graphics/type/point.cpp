#include "point.h"

#include "graphics/glvao.h"

namespace graphics {

void Point::setupVao(GlVao &vao) {
    GLuint positionLocation = vao.prepareProgramAttribute("POSITION_LOCATION", 1);
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void *>(offsetof(Point, position)));
    glEnableVertexAttribArray(positionLocation);
    graphics::GL::catchErrors();
}

}

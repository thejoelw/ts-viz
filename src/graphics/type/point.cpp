#include "point.h"

#include "graphics/glvao.h"

namespace graphics {

void PointShared::setupVao(GlVao &vao) {
    // glVertexAttribPointer
    GLuint positionLocation = vao.prepareProgramAttribute("POSITION_LOCATION", 1);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(PointShared), reinterpret_cast<void *>(offsetof(PointShared, position)));
    glEnableVertexAttribArray(positionLocation);
    graphics::GL::catchErrors();

    GLuint normalLocation = vao.prepareProgramAttribute("NORMAL_LOCATION", 1);
    glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(PointShared), reinterpret_cast<void *>(offsetof(PointShared, normal)));
    glEnableVertexAttribArray(normalLocation);
    graphics::GL::catchErrors();

    GLuint meshIndexLocation = vao.prepareProgramAttribute("MESH_INDEX_LOCATION", 1);
    glVertexAttribIPointer(meshIndexLocation, 1, GL_UNSIGNED_INT, sizeof(PointShared), reinterpret_cast<void *>(offsetof(PointShared, meshIndex)));
    glEnableVertexAttribArray(meshIndexLocation);
    graphics::GL::catchErrors();

    GLuint materialIndexLocation = vao.prepareProgramAttribute("MATERIAL_INDEX_LOCATION", 1);
    glVertexAttribIPointer(materialIndexLocation, 1, GL_UNSIGNED_INT, sizeof(PointShared), reinterpret_cast<void *>(offsetof(PointShared, materialIndex)));
    glEnableVertexAttribArray(materialIndexLocation);
    graphics::GL::catchErrors();
}

}

#pragma once

#include "graphics/gl.h"

namespace graphics {

class GlVao;

class PointShared {
public:
    GLfloat position[3];
    GLfloat normal[3];

    GLuint meshIndex;
    GLuint materialIndex = 0;

    static void setupVao(GlVao &vao);
};

class PointLocal {
public:
};

}

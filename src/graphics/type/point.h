#pragma once

#include "graphics/gl.h"

namespace graphics {

class GlVao;

class Point {
public:
    GLfloat position[2];

    static void setupVao(GlVao &vao);
};

}

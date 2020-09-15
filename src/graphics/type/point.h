#pragma once

#include "graphics/gl.h"
#include "render/program/program.h"

namespace graphics {

class GlVao;

class Point {
public:
    GLfloat position[2];

    static void insertDefines(render::Program::Defines &defines);
};

}

#pragma once

#include "graphics/gl.h"
#include "render/program/program.h"

namespace graphics {

template <typename ElementType>
class Element {
public:
    ElementType position;

    static void insertDefines(render::Program::Defines &defines);
};

static_assert(sizeof(Element<float>) == sizeof(float), "Bad padding");
static_assert(sizeof(Element<double>) == sizeof(double), "Bad padding");

}

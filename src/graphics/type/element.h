#pragma once

#include "graphics/gl.h"

namespace graphics {

class GlVao;

template <typename ElementType>
class Element {
public:
    ElementType position;

    static void setupVao(GlVao &vao);
};

static_assert(sizeof(Element<float>) == sizeof(float), "Bad padding");
static_assert(sizeof(Element<double>) == sizeof(double), "Bad padding");

}

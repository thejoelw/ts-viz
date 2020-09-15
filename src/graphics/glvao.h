#pragma once

#include <vector>

#include "graphics/gl.h"
#include "graphics/gpuprogram.h"

namespace graphics {

class GlVao {
public:
    GlVao();
    ~GlVao();

    GLuint getId() const {
        return vaoId;
    }

    void bind() const;
    void unbind() const;
    void assertBound() const;

private:
    GLuint vaoId;
};

}

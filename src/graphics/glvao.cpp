#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "glvao.h"

#include "defs/GLVAO_ASSERT_BINDINGS.h"

namespace graphics {

GlVao::GlVao() {
    glGenVertexArrays(1, &vaoId);
}

GlVao::~GlVao() {
    glDeleteVertexArrays(1, &vaoId);
}

void GlVao::bind() const {
    glBindVertexArray(vaoId);
}

void GlVao::unbind() const {
    glBindVertexArray(0);
}

void GlVao::assertBound() const {
#if GLVAO_ASSERT_BINDINGS
    GLint binding;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &binding);
    assert(static_cast<GLuint>(binding) == vaoId);
#endif
}

}

#endif

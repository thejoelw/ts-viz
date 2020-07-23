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

    GLuint prepareProgramAttribute(const std::string &name, GLuint locationSize);
    void prepareDefine(const std::string &name, GLuint value);

    void insertDefines(GpuProgram::Defines &defines) const;

private:
    GLuint vaoId;
    GLuint nextAttributeLocation = 0;

    struct Define {
        std::string name;
        GLuint value;
    };

    std::vector<Define> preparedDefines;
};

}

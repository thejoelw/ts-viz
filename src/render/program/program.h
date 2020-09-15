#pragma once

#include "app/appcontext.h"

#include "graphics/gpuprogram.h"

namespace graphics { class GlBufferBase; }

namespace render {

class Program : public graphics::GpuProgram {
public:
    class Defines : public graphics::GpuProgram::Defines {
    public:
        GLuint addProgramAttribute(const std::string &name, GLuint locationSize) {
            for (const ProgramAttribute &attr : attrs) {
                if (attr.name == name) {
                    return attr.value;
                }
            }

            ProgramAttribute attr;
            attr.name = name;
            attr.value = nextAttributeLocation;
            attrs.push_back(attr);

            nextAttributeLocation += locationSize;

            int maxAttributes;
            glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttributes);
            assert(nextAttributeLocation <= static_cast<unsigned int>(maxAttributes));

            set(attr.name, attr.value);

            return attr.value;
        }

    private:
        GLuint nextAttributeLocation = 0;

        struct ProgramAttribute {
            std::string name;
            GLuint value;
        };
        std::vector<ProgramAttribute> attrs;
    };

    Program(app::AppContext &context);
    virtual ~Program() {}

    void make();

protected:
    Defines defines;

    virtual void insertDefines();
    virtual void setupProgram();
    virtual void linkProgram();

private:
#ifndef NDEBUG
    // Make sure each derived class calls BaseClass::insertDefines(defines) and BaseClass::setupProgram(defines)
    bool calledBaseInsertDefines;
    bool calledBaseSetupProgram;
    bool calledBaseLinkProgram;
#endif
};

}

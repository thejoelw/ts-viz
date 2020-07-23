#pragma once

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <assert.h>
#include <string>

#include "jw_util/baseexception.h"

namespace graphics {

class GL {
public:
    class Exception : public jw_util::BaseException {
        friend class GL;

    private:
        Exception(const std::string &msg);
        ~Exception();
    };

    static void catchErrors() {
#ifndef NDEBUG
        GLenum err = glGetError();
        if (err) {
            std::string strings = errorString(err);
            while ((err = glGetError())) {
                strings += ", " + errorString(err);
            }
            assert(false);
            throw Exception("GL errors: " + strings);
        }
#endif
    }

    static std::string getErrors() {
        GLenum err = glGetError();
        if (err) {
            std::string strings = errorString(err);
            while ((err = glGetError())) {
                strings += ", " + errorString(err);
            }
            return strings;
        } else {
            return std::string();
        }
    }

private:
    static std::string errorString(GLenum code) {
        switch (code) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR ???";

        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";

        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";

        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";

        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";

        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";

        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";

        default:
            return "unknown (" + std::to_string(code) + ")";
        }
    }
};

}

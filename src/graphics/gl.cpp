#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS

#include "gl.h"

namespace graphics {

GL::Exception::Exception(const std::string &msg)
    : BaseException(msg)
{}

GL::Exception::~Exception() {}

}

#endif

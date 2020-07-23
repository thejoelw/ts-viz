#include "shaders.h"

namespace render {

const char Shaders::mainVert[] = {
    #include "shaders/main.vert.glsl.h"
};

const char Shaders::mainFrag[] = {
    #include "shaders/main.frag.glsl.h"
};

}

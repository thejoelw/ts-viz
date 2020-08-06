#include "shaders.h"

namespace render {

const char Shaders::mainVert[] = {
    #include "shaders/main.vert.glsl.h"
};

const char Shaders::mainFrag[] = {
    #include "shaders/main.frag.glsl.h"
};

const char Shaders::fillVert[] = {
    #include "shaders/fill.vert.glsl.h"
};

const char Shaders::fillFrag[] = {
    #include "shaders/fill.frag.glsl.h"
};

}

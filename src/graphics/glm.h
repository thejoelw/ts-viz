#pragma once

#include "defs/ENABLE_GRAPHICS.h"
static_assert(ENABLE_GRAPHICS, "Should not be including graphics/glm.h if ENABLE_GRAPHICS is false");

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_NO_CTOR_INIT
//#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_ENABLE_EXPERIMENTAL

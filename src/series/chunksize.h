#pragma once

#include <climits>

#include "defs/CHUNK_SIZE_LOG2.h"
static_assert(CHUNK_SIZE_LOG2 < sizeof(unsigned int) * CHAR_BIT - 1, "Lots of things depend on an inter-chunk index fitting into a signed int");
#define CHUNK_SIZE (1u << CHUNK_SIZE_LOG2)

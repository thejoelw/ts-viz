#include "version.h"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

const char *tsVizVersion = STRINGIFY(TSVIZ_VERSION);

#include "program.h"

namespace render {

Program::Program(app::AppContext &context)
    : GpuProgram(context)
{}

void Program::make() {
    Defines defines;

#ifndef NDEBUG
    calledBaseInsertDefines = false;
    calledBaseSetupProgram = false;
    calledBaseLinkProgram = false;
#endif

    insertDefines(defines);
    assert(calledBaseInsertDefines);

    setupProgram(defines);
    assert(calledBaseSetupProgram);

    linkProgram();
    assert(calledBaseLinkProgram);
}

void Program::insertDefines(Defines &defines) {
#ifndef NDEBUG
    calledBaseInsertDefines = true;
#endif
}

void Program::setupProgram(const Defines &defines) {
#ifndef NDEBUG
    calledBaseSetupProgram = true;
#endif
}

void Program::linkProgram() {
#ifndef NDEBUG
    calledBaseLinkProgram = true;
#endif

    link();
}

}

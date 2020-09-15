#include "program.h"

namespace render {

Program::Program(app::AppContext &context)
    : GpuProgram(context)
{}

void Program::make() {
#ifndef NDEBUG
    calledBaseInsertDefines = false;
    calledBaseSetupProgram = false;
    calledBaseLinkProgram = false;
#endif

    insertDefines();
    assert(calledBaseInsertDefines);

    if (isLinked) {
        return;
    }

    setupProgram();
    assert(calledBaseSetupProgram);

    linkProgram();
    assert(calledBaseLinkProgram);
}

void Program::insertDefines() {
#ifndef NDEBUG
    calledBaseInsertDefines = true;
#endif
}

void Program::setupProgram() {
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

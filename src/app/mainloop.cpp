#include "mainloop.h"

#include <unistd.h>

#include "app/tickercontext.h"
#include "program/programmanager.h"
#include "stream/inputmanager.h"
#include "stream/outputmanager.h"

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    do {
        context.get<TickerContext>().tick();
    } while (shouldRun());
}

bool MainLoop::shouldRun() const {
    return false
            || context.get<program::ProgramManager>().isRunning()
            || context.get<stream::InputManager>().isRunning()
            || context.get<stream::OutputManager>().isRunning();
}

}

#include "mainloop.h"

#include <unistd.h>

#include "app/tickercontext.h"
#include "app/window.h"
#include "program/executor.h"
#include "render/renderer.h"
#include "render/axes.h"

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    context.get<Window>();
    context.get<program::Executor>();
    context.get<render::Renderer>();
    context.get<render::Axes>();

    while (true) {
        context.get<TickerContext>().tick();
    }
}

}

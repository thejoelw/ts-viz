#include "mainloop.h"

#include <unistd.h>

#include "app/tickercontext.h"
#include "app/window.h"

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    context.get<Window>();

    while (true) {
        context.get<TickerContext>().tick();
        usleep(100000);
    }
}

}

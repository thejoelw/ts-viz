#include "mainloop.h"

#include <unistd.h>

#include "app/tickercontext.h"

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    while (true) {
        context.get<TickerContext>().tick();
    }
}

}

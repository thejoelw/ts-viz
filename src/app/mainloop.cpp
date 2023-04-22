#include "mainloop.h"

#include "defs/ENABLE_GRAPHICS.h"

#include <unistd.h>

#include "app/tickercontext.h"
#include "program/programmanager.h"
#include "stream/inputmanager.h"
#include "stream/metricmanager.h"
#include "app/options.h"

#if ENABLE_GRAPHICS
#include "app/window.h"
#endif

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    do {
        std::size_t maxFps =
#if ENABLE_GRAPHICS
                context.get<Window>().shouldRender() ? app::Options::getInstance().maxFps : 10
#else
                app::Options::getInstance().maxFps
#endif
                ;

        std::chrono::steady_clock::time_point timeout;
        if (maxFps != 0) {
            timeout = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)) / maxFps;
        }

        context.get<TickerContext>().tick();

        if (maxFps != 0 && std::chrono::steady_clock::now() < timeout) {
            std::this_thread::sleep_until(timeout);
        }
    } while (shouldRun());
}

bool MainLoop::shouldRun() const {
    return false
            || app::Options::getInstance().dontExit
            || context.get<program::ProgramManager>().isRunning()
            || context.get<stream::InputManager>().isRunning()
            || context.get<stream::MetricManager>().isRunning();
}

}

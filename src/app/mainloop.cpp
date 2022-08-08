#include "mainloop.h"

#include <unistd.h>

#include "app/tickercontext.h"
#include "program/programmanager.h"
#include "stream/inputmanager.h"
#include "stream/metricmanager.h"
#include "app/options.h"

namespace app {

MainLoop::MainLoop(AppContext &context)
    : context(context)
{}

void MainLoop::run() {
    bool enableFpsCap = app::Options::getInstance().maxFps != 0;
    std::chrono::duration sleepDuration = std::chrono::seconds(0);
    if (enableFpsCap) {
        sleepDuration = std::chrono::seconds(1) / app::Options::getInstance().maxFps;
    }

    std::chrono::steady_clock::time_point timeout;
    do {
        if (enableFpsCap) {
            if (std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_until(timeout);
            }
            timeout = std::chrono::steady_clock::now() + sleepDuration;
        }

        context.get<TickerContext>().tick();
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

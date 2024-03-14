#include "mainloop.h"

#include "defs/ENABLE_GRAPHICS.h"

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

        if (maxFps != 0) {
            blockTimeout = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)) / maxFps;
        } else {
            blockTimeout = std::chrono::steady_clock::time_point();
        }

        context.get<TickerContext>().tick();

        if (blockTimeout != std::chrono::steady_clock::time_point() && std::chrono::steady_clock::now() < blockTimeout) {
            std::this_thread::sleep_until(blockTimeout);
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

std::chrono::steady_clock::duration MainLoop::getBlockDuration() {
    if (blockTimeout != std::chrono::steady_clock::time_point()) {
        std::chrono::steady_clock::duration res = blockTimeout - std::chrono::steady_clock::now();
        if (res > std::chrono::steady_clock::duration::zero()) {
            return res;
        } else {
            blockTimeout = std::chrono::steady_clock::time_point();
            return std::chrono::steady_clock::duration::zero();
        }
    } else {
        return std::chrono::steady_clock::duration::zero();
    }
}

}

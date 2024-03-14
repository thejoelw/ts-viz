#pragma once

#include <chrono>

namespace app {

class AppContext;

class MainLoop {
public:
    MainLoop(AppContext &context);

    void run();

    std::chrono::steady_clock::duration getBlockDuration();

private:
    AppContext &context;

    std::chrono::steady_clock::time_point blockTimeout;

    bool shouldRun() const;
};

}

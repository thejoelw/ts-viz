#pragma once

namespace app {

class AppContext;

class MainLoop {
public:
    MainLoop(AppContext &context);

    void run();

private:
    AppContext &context;
};

}

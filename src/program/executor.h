#pragma once

#include "taskflow/taskflow/taskflow.hpp"

#include "app/tickercontext.h"

namespace program {

class Executor : app::TickerContext::TickableBase<Executor> {
public:
    Executor(app::AppContext &context);
    ~Executor();

    void tick(app::TickerContext &tickerContext);

private:
    tf::Executor executor;
    tf::Taskflow taskflow;
};

}

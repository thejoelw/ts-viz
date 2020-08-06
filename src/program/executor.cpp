#include "executor.h"

#include "app/appcontext.h"
#include "series/series.h"

namespace program {

Executor::Executor(app::AppContext &context)
    : TickableBase(context)
{
    context.provideInstance<tf::Taskflow>(&taskflow);
}

Executor::~Executor() {
    context.removeInstance<tf::Taskflow>();
}

void Executor::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    auto layers = context.get<series::Series::Registry>().getLayers();
    auto it = layers.crbegin();
    while (it != layers.crend()) {
        util::RefSet<series::Series>::Invoker<>::call<&series::Series::propogateRequest>(*it);
        it++;
    }

    executor.run(taskflow).wait();
}

}

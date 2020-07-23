#include "tickercontext.h"

namespace app {

void TickerContext::tick() {
    assert(getManagedTypeCount() == 0);
    assert(getTotalTypeCount() == 0);
    builder.buildAll(*this);
    reset();
}

void TickerContext::log(LogLevel level, const std::string &msg) {
    (void) level;
    (void) msg;
}

}

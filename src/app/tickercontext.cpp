#include "tickercontext.h"

namespace app {

void TickerContext::tick() {
#if PRINT_TICK_ORDER
    spdlog::debug("Tick order: Enter global tick");
#endif

    assert(getManagedTypeCount() == 0);
    assert(getTotalTypeCount() == 0);
    builder.buildAll(*this);
    reset();

#if PRINT_TICK_ORDER
    spdlog::debug("Tick order: Exit global tick");
#endif
}

void TickerContext::log(LogLevel level, const std::string &msg) {
    (void) level;
    (void) msg;
}

}

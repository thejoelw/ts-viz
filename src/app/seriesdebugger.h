#pragma once

#include "app/tickercontext.h"

namespace app {

class AppContext;

class SeriesDebugger : public app::TickerContext::TickableBase<SeriesDebugger> {
public:
    SeriesDebugger(AppContext &context);

    void tick(app::TickerContext &tickerContext);
};

}

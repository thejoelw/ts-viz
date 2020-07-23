#pragma once

#include "app/tickercontext.h"

namespace app {

class AppContext;

class SignalHandler : public TickerContext::TickableBase<SignalHandler> {
public:
    SignalHandler(AppContext &context);
    ~SignalHandler();

    void tick(TickerContext &tickerContext);
};

}

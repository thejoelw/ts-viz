#pragma once

#include "app/tickercontext.h"

namespace render {

class Axes : public app::TickerContext::TickableBase<Axes, app::TickerContext::ScopedCaller<Axes>> {
public:
    Axes(app::AppContext &context);

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);

private:
};

}

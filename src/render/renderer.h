#pragma once

#include <vector>

#include "app/tickercontext.h"
#include "series/series.h"

namespace render {

class Renderer : public app::TickerContext::TickableBase<Renderer, app::TickerContext::ScopedCaller<Renderer>> {
public:
    Renderer(app::AppContext &context);

    void clearSeries();
    void addSeries(const char *name, series::Series *s);

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);

private:
    std::vector<series::Series *> added;
};

}

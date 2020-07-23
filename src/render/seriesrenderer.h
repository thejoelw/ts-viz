#pragma once

#include <deque>

#include "app/tickercontext.h"
#include "series/series.h"

namespace render {

class SeriesRenderer : public app::TickerContext::TickableBase<SeriesRenderer, app::TickerContext::ScopedCaller<SeriesRenderer>> {
public:
    SeriesRenderer(app::AppContext &context);

    void addSeries(const char *name, const series::Series<float> &s);

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);
};

}

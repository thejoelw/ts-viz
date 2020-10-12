#pragma once

#include <vector>

#include "app/tickercontext.h"
#include "render/seriesrenderer.h"

namespace render {

class Renderer : public app::TickerContext::TickableBase<Renderer, app::TickerContext::ScopedCaller<Renderer>> {
public:
    Renderer(app::AppContext &context);

    void clearSeries();
    void addSeries(render::SeriesRenderer *renderer);

    void tickOpen(app::TickerContext &tickerContext);
    void tickClose(app::TickerContext &tickerContext);

private:
    std::vector<render::SeriesRenderer *> added;
};

}

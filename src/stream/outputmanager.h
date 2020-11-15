#pragma once

#include <vector>

#include "app/tickercontext.h"
#include "stream/seriesemitter.h"

namespace stream {

class OutputManager : public app::TickerContext::TickableBase<OutputManager> {
public:
    OutputManager(app::AppContext &context)
        : TickableBase(context)
    {}

    void clearEmitters();
    void addEmitter(SeriesEmitter *emitter);

    void tick(app::TickerContext &tickerContext);

private:
    std::size_t nextEmitIndex = 0;

    std::vector<SeriesEmitter *> emitters;
};

}

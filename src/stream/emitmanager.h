#pragma once

#include <vector>

#include "app/tickercontext.h"
#include "stream/seriesemitter.h"

namespace stream {

class EmitManager : public app::TickerContext::TickableBase<EmitManager> {
public:
    EmitManager(app::AppContext &context);
    ~EmitManager();

    void clearEmitters();
    void addEmitter(SeriesEmitter *emitter);

    void tick(app::TickerContext &tickerContext);

private:
    std::size_t nextEmitIndex = 0;

    std::vector<SeriesEmitter *> curEmitters;

    void emit();
};

}

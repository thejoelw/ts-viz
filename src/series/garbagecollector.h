#pragma once

#include "app/tickercontext.h"

namespace series {

class GarbageCollector : public app::TickerContext::TickableBase<GarbageCollector> {
public:
    GarbageCollector(app::AppContext &context);

    void setMemoryLimit(std::size_t limit) {
        memoryLimit = limit;
    }

    void tick(app::TickerContext &tickerContext);

private:
    std::size_t memoryLimit = 0;

    std::size_t estimateMemoryUsage() const;
    void freeMemory(std::size_t bytes);
};

}

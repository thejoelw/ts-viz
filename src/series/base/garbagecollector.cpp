#include "garbagecollector.h"

namespace series {

GarbageCollector::GarbageCollector(app::AppContext &context)
    : TickableBase(context)
{}

// dependents could be null, in which case we need to skip it.

void GarbageCollector::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    std::size_t usage = estimateMemoryUsage();
    if (usage > memoryLimit) {
        freeMemory(usage - memoryLimit);
    }
}

std::size_t GarbageCollector::estimateMemoryUsage() const {
    return 0;
}

void GarbageCollector::freeMemory(std::size_t bytes) {
}

}

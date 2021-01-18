#include "garbagecollector.h"

#include "series/chunkbase.h"

namespace series {

GarbageCollector::GarbageCollector(app::AppContext &context)
    : TickableBase(context)
{}

void GarbageCollector::tick(app::TickerContext &tickerContext) {
    jw_util::Thread::assert_main_thread();
    (void) tickerContext;

    return;

    currentTime++;

    if (memoryUsage > memoryLimit) {
        ChunkIterator it;

        for (std::pair<void (*)(void *, ChunkIterator &), void *> col : dsCollections) {
            col.first(col.second, it);
        }

        while (!it.queue.empty() && memoryUsage > memoryLimit) {
            // TODO: What happens if a chunk is running while this tries to delete it?
            // We could just increment the counter while it's running, but that's not atomic.
            // We could only decrement the counter when the chunk finishes computing, but them we can't delete in-progress chunks.
            *it.queue.top() = ChunkPtrBase::null();
            it.queue.pop();
        }
    }
}

void GarbageCollector::updateMemoryUsage(std::make_signed<std::size_t>::type inc) {
    jw_util::Thread::assert_main_thread();

#ifndef NDEBUG
    if (inc < 0) {
        assert(static_cast<std::make_signed<std::size_t>::type>(memoryUsage) >= -inc);
    }
#endif

    memoryUsage += inc;
}

}

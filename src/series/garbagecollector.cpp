#include "garbagecollector.h"

#include "app/options.h"
#include "series/chunkbase.h"
#include "log.h"

namespace series {

GarbageCollector::GarbageCollector(app::AppContext &context)
    : TickableBase(context)
{}

void GarbageCollector::tick(app::TickerContext &tickerContext) {
    jw_util::Thread::assert_main_thread();
    (void) tickerContext;

    currentTime++;

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    static thread_local unsigned int ctr = 0;
    ctr++;
    if (ctr % 65536 == 0) {
        SPDLOG_TRACE("Memory usage is at {} / {}", memoryUsage, app::Options::getInstance().gcMemoryLimit);
    }
#endif

    runGc();
}

void GarbageCollector::updateMemoryUsage(std::make_signed<std::size_t>::type inc) {
    jw_util::Thread::assert_main_thread();

    if (inc >= 0) {
        assert(inc != 0);
        runGc();
    } else {
        assert(static_cast<std::make_signed<std::size_t>::type>(memoryUsage) >= -inc);
    }

    memoryUsage += inc;
}

void GarbageCollector::runGc() {
    std::size_t prevUsage = memoryUsage;
    std::size_t memoryLimit = app::Options::getInstance().gcMemoryLimit;

    if (prevUsage > memoryLimit) {
        static thread_local unsigned int backoffCtr = 0;
        static thread_local unsigned int backoffStart = 1;
        if (backoffCtr--) {
            return;
        }

        ChunkIterator it;

        for (std::pair<void (*)(void *, ChunkIterator &), void *> col : dsCollections) {
            col.first(col.second, it);
        }

        // Delete some extra stuff so we don't have to do this again soon
        memoryLimit = static_cast<std::uint64_t>(memoryLimit) * 15 / 16;

        SPDLOG_DEBUG("Running GC; we have {} candidate chunks, trying to delete at least {} bytes", it.queue.size(), prevUsage - memoryLimit);

        unsigned int deleted = 0;
        while (!it.queue.empty() && memoryUsage > memoryLimit) {
            // TODO: What happens if a chunk is running while this tries to delete it?
            // We could just increment the ref counter while it's running, but that's not atomic. Actually it is.
            // We could only decrement the counter when the chunk finishes computing, but them we can't delete in-progress chunks.
            *it.queue.top() = ChunkPtrBase::null();
            it.queue.pop();
            deleted++;
        }

        SPDLOG_DEBUG("Finished GC; deleted {} chunks, and {} bytes", deleted, prevUsage - memoryUsage);

        if (deleted) {
            backoffCtr = 0;
            backoffStart /= 2;
            if (backoffStart < 1) {
                backoffStart = 1;
            }
        } else {
            backoffCtr = backoffStart;
            backoffStart *= 2;
            if (backoffStart > 65536) {
                backoffStart = 65536;
            }
        }
    }
}

}

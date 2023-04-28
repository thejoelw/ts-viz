#include "garbagecollector.h"

#include "app/options.h"
#include "program/programmanager.h"
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
        ChunkReleaseQueue queue;

        // If we're not going to get any more novel programs, we can assume unreferenced inputs won't be referenced again
        bool canReleaseInputs = !context.get<program::ProgramManager>().isRunning();

        for (std::pair<void (*)(void *, ChunkReleaseQueue &, bool), void *> col : dsCollections) {
            col.first(col.second, queue, canReleaseInputs);
        }

        // Delete some extra stuff so we don't have to do this again soon
        memoryLimit = static_cast<std::uint64_t>(memoryLimit) * 15 / 16;

        SPDLOG_DEBUG("Running GC; we have {} data series and {} candidate chunks, trying to delete at least {} bytes", dsCollections.size(), queue.size(), prevUsage - memoryLimit);

        unsigned int deleted = 0;
        while (!queue.empty() && memoryUsage > memoryLimit) {
            // TODO: What happens if a chunk is running while this tries to delete it?
            // We could just increment the ref counter while it's running, but that's not atomic. Actually it is.
            // We could only decrement the counter when the chunk finishes computing, but then we can't delete in-progress chunks.

            const ChunkAddress &addr = queue.top();
            SPDLOG_DEBUG("Deleting chunk with access time {}", addr.lastAccess);
            addr.release(addr.dsPtr, addr.index);
            queue.pop();
            deleted++;
        }

        SPDLOG_DEBUG("Finished GC; deleted {} chunks, and {} bytes", deleted, prevUsage - memoryUsage);
    }
}

}

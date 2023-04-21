#include "chunkbase.h"

#include "app/appcontext.h"
#include "series/dataseriesbase.h"
#include "util/taskscheduler.h"
#include "series/garbagecollector.h"

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"
#include "defs/ENABLE_NOTIFICATION_TRACING.h"

#if ENABLE_NOTIFICATION_TRACING
#include "log.h"
#endif

namespace series {

ChunkBase::ChunkBase(DataSeriesBase *ds)
    : ds(ds)
    , followingDuration(NAN)
{
    jw_util::Thread::assert_main_thread();
    recordAccess();
}

ChunkBase::~ChunkBase() {
#if ENABLE_CHUNK_MULTITHREADING
#ifndef NDEBUG
    // Gotta stop threads before destructing stuff
    assert(notifies == 0);
#endif
#endif
}

void ChunkBase::addDependent(ChunkPtrBase dep) {
    jw_util::Thread::assert_main_thread();
    assert(dep.operator->() != this);

#if ENABLE_CHUNK_MULTITHREADING
    std::lock_guard<util::SpinLock> lock(mutex);
#endif
    dependents.push_back(std::move(dep));
}

void ChunkBase::removeDependent(ChunkBase *chunk) {
    jw_util::Thread::assert_main_thread();

#if ENABLE_CHUNK_MULTITHREADING
    std::lock_guard<util::SpinLock> lock(mutex);
#endif

    for (std::vector<ChunkPtrBase>::iterator i = dependents.begin(); i != dependents.end(); i++) {
        if (i->operator->() == chunk) {
            *i = std::move(dependents.back());
            dependents.pop_back();
            return;
        }
    }

    // If this triggers, the dependent doesn't exist. This might mean chunk construction isn't deterministic.
    assert(false);
}

#if ENABLE_CHUNK_MULTITHREADING
std::chrono::duration<float> ChunkBase::getOrdering() const {
    return getCriticalPathDuration();
}

std::chrono::duration<float> ChunkBase::getCriticalPathDuration() const {
    if (std::isnan(followingDuration.count())) {
        followingDuration = std::chrono::duration<float>::zero();
        for (const ChunkPtrBase &dep : dependents) {
            std::chrono::duration<float> depDur = dep->getCriticalPathDuration();
            if (depDur > followingDuration) {
                followingDuration = depDur;
            }
        }
    }
    return ds->getAvgRunDuration() + followingDuration;
}
#endif

void ChunkBase::notify() {
#if ENABLE_NOTIFICATION_TRACING
    SPDLOG_TRACE(getIndentation(2) + "{}.notify() {{", name);
#endif

#if ENABLE_CHUNK_MULTITHREADING
    unsigned int n = notifies++;
#if ENABLE_NOTIFICATION_TRACING
    SPDLOG_TRACE(getIndentation(0) + "previousNotifies: {}", n);
#endif

    if (n == 0) {
#if ENABLE_NOTIFICATION_TRACING
        SPDLOG_TRACE(getIndentation(0) + "isDone: {}", isDone());
#endif
        if (!isDone()) {
            static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
            bool runInThread = ds->getAvgRunDuration() > taskLengthThreshold;
#if ENABLE_NOTIFICATION_TRACING
            SPDLOG_TRACE(getIndentation(0) + "runInThread: {}", runInThread);
#endif
            if (runInThread) {
                ds->getContext().get<util::TaskScheduler<ChunkBase>>().addTask(this);
            } else {
                exec();
            }
        }
    }
#else
#if ENABLE_NOTIFICATION_TRACING
    SPDLOG_TRACE(getIndentation(0) + "isDone: {}", isDone());
#endif
    if (!isDone()) {
        exec();
    }
#endif

#if ENABLE_NOTIFICATION_TRACING
    SPDLOG_TRACE(getIndentation(-2) + "}} // {}.notify()", name);
#endif
}

void ChunkBase::recordAccess() {
    lastAccessTime = GarbageCollector::getCurrentTime();
}
unsigned int ChunkBase::getLastAccess() const {
    return lastAccessTime;
}

void ChunkBase::incRefs() {
    refs++;
    recordAccess();
}
void ChunkBase::decRefs() {
    assert(refs > 0);
    if (--refs == 0) {
        delete this;
    }
}

void ChunkBase::updateMemoryUsage(std::make_signed<std::size_t>::type inc) {
    ds->getContext().get<GarbageCollector>().updateMemoryUsage(inc);
}

}

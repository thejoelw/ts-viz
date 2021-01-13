#include "chunkbase.h"

#include "app/appcontext.h"
#include "series/dataseriesbase.h"
#include "util/taskscheduler.h"

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

#if ENABLE_CHUNK_NAMES
#include "log.h"
#endif

namespace series {

ChunkBase::ChunkBase(DataSeriesBase *ds)
    : ds(ds)
    , followingDuration(NAN)
{
    jw_util::Thread::assert_main_thread();
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
#if ENABLE_CHUNK_NAMES
    SPDLOG_TRACE(getIndentation(2) + "{}.notify() {{", name);
#endif

#if ENABLE_CHUNK_MULTITHREADING
    unsigned int n = notifies++;
#if ENABLE_CHUNK_NAMES
    SPDLOG_TRACE(getIndentation(0) + "previousNotifies: {}", n);
#endif

    if (n == 0) {
#if ENABLE_CHUNK_NAMES
        SPDLOG_TRACE(getIndentation(0) + "isDone: {}", isDone());
#endif
        if (!isDone()) {
            static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
            bool runInThread = ds->getAvgRunDuration() > taskLengthThreshold;
#if ENABLE_CHUNK_NAMES
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
#if ENABLE_CHUNK_NAMES
    SPDLOG_TRACE(getIndentation(0) + "isDone: {}", isDone());
#endif
    if (!isDone()) {
        exec();
    }
#endif

#if ENABLE_CHUNK_NAMES
    SPDLOG_TRACE(getIndentation(-2) + "}} // {}.notify()", name);
#endif
}

void ChunkBase::recordAccess() {
    lastAccess = std::chrono::steady_clock::now();
}
ChunkBase::AccessInstant ChunkBase::getLastAccess() const {
    return lastAccess;
}

void ChunkBase::incRefs() {
    refs++;
}
void ChunkBase::decRefs() {
    if (--refs == 0) {
        delete this;
    }
}

}

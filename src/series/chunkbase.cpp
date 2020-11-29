#include "chunkbase.h"

#include "app/appcontext.h"
#include "series/dataseriesbase.h"
#include "util/taskscheduler.h"

namespace series {

ChunkBase::ChunkBase(DataSeriesBase *ds)
    : ds(ds)
    , followingDuration(NAN)
{
    jw_util::Thread::assert_main_thread();
}

ChunkBase::~ChunkBase() {
#ifndef NDEBUG
    // Gotta stop threads before destructing stuff
    assert(notifies == 0);
#endif
}

void ChunkBase::addDependent(ChunkPtrBase dep) {
    assert(dep.operator->() != this);

    std::lock_guard<util::SpinLock> lock(mutex);
    dependents.push_back(std::move(dep));
}

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

void ChunkBase::notify() {
    if (notifies++ == 0) {
        if (!isDone()) {
            static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
            if (ds->getAvgRunDuration() > taskLengthThreshold) {
                ds->getContext().get<util::TaskScheduler<ChunkBase>>().addTask(this);
            } else {
                exec();
            }
        }
    }
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

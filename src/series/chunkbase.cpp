#include "chunkbase.h"

#include "app/appcontext.h"
#include "series/chunksize.h"
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
    assert(notifies == 0);
}

void ChunkBase::addDependent(ChunkPtrBase dep) {
    assert(dep.operator->() != this);

    std::lock_guard<util::SpinLock> lock(mutex);
    dependents.push_back(std::move(dep));
}

unsigned int ChunkBase::getComputedCount() const {
    return computedCount;
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
        if (computedCount == CHUNK_SIZE) {
            notifies--;
        } else {
            static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
            if (ds->getAvgRunDuration() > taskLengthThreshold) {
                ds->getContext().get<util::TaskScheduler<ChunkBase>>().addTask(this);
            } else {
                exec();
            }
        }
    }
}

void ChunkBase::exec() {
    assert(computedCount < CHUNK_SIZE);

    unsigned int prevNotifies = notifies;

    auto t1 = std::chrono::high_resolution_clock::now();
    unsigned int prevCount = computedCount;
    unsigned int count = computer(prevCount);
    auto t2 = std::chrono::high_resolution_clock::now();

    // Set this first so the dependents we notify know what we've computed.
    assert(count >= prevCount);
    computedCount = count;

    if (notifies.exchange(0) == prevNotifies || count == CHUNK_SIZE) {
        if (count == CHUNK_SIZE) {
            // Destroy any smart_ptrs that the lambda had captured.
            // This allows dependency chunks to be destroyed.
            computer = fu2::unique_function<unsigned int (unsigned int)>();
        }

        // No notifies came in while we were computing.
        // Notifies is zero. The next notify() will relaunch exec().

        if (count != prevCount) {
            static thread_local std::vector<ChunkPtrBase> processingDeps;
            std::size_t prevSize = processingDeps.size();

            {
                std::lock_guard<util::SpinLock> lock(mutex);
                for (ChunkPtrBase &dep : dependents) {
                    processingDeps.emplace_back(std::move(dep));
                }
            }

            for (const ChunkPtrBase &dep : processingDeps) {
                dep->notify();
            }

            {
                std::lock_guard<util::SpinLock> lock(mutex);
                for (std::size_t i = processingDeps.size() - prevSize; i-- > 0;) {
                    dependents[i] = std::move(processingDeps.back());
                    processingDeps.pop_back();
                }
            }
        }
    } else {
        // One or more notifies came in while we were computing.
        // Either this call will re-launch exec(), or something came already and exec() is already running.
        notify();
    }

    if (count != prevCount) {
        ds->recordDuration(std::chrono::duration(t2 - t1) / (count - prevCount));
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
        ds->destroyChunk(this);
    }
}

void ChunkBase::setComputer(fu2::unique_function<unsigned int (unsigned int)> &&func) {
    computer = std::move(func);
}

}

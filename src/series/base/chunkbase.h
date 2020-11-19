#pragma once

#include <chrono>
#include <cmath>
#include <vector>

#include "function2/include/function2/function2.hpp"

#include "jw_util/thread.h"

#include "series/base/chunkptrbase.h"
#include "util/spinlock.h"

namespace series { class DataSeriesBase; }

namespace series {

class ChunkBase {
public:
    typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>> AccessInstant;
    static_assert(sizeof(AccessInstant) == 4, "Unexpected sizeof(AccessInstant), expecting 4!");

    ChunkBase(DataSeriesBase *ds);
    ~ChunkBase();

    void addDependent(ChunkPtrBase dep);

    unsigned int getComputedCount() const;

    std::chrono::duration<float> getOrdering() const;
    std::chrono::duration<float> getCriticalPathDuration() const;

    void notify();
    void exec();

    void recordAccess();
    AccessInstant getLastAccess() const;

    void incRefs();
    void decRefs();

protected:
    void setComputer(fu2::unique_function<unsigned int (unsigned int)> &&func);

    DataSeriesBase *ds;

    AccessInstant lastAccess;
    std::atomic<unsigned int> refs = 0;

    std::atomic<unsigned int> computedCount = 0;
    std::atomic<unsigned int> notifies = 0;
    util::SpinLock mutex;
    std::vector<ChunkPtrBase> dependents;

    mutable std::chrono::duration<float> followingDuration;

    fu2::unique_function<unsigned int (unsigned int)> computer;
};

}

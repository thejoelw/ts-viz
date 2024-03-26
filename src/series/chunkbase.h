#pragma once

#include <chrono>
#include <cmath>
#include <vector>
#include <atomic>

#include "log.h"
#include "jw_util/thread.h"

#include "series/chunkptrbase.h"
#include "series/garbagecollector.h"

#include "defs/ENABLE_CHUNK_DEBUG.h"
#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

#if ENABLE_CHUNK_MULTITHREADING
#include "util/spinlock.h"
#endif

#if ENABLE_CHUNK_DEBUG
#include <string>
#endif

namespace series { class DataSeriesBase; }

namespace series {

class ChunkBase {
public:
    ChunkBase(DataSeriesBase *ds);
    virtual ~ChunkBase();

    void addDependent(ChunkBase *dep);
    void removeDependent(const ChunkBase *dep);

#if ENABLE_CHUNK_MULTITHREADING
    std::chrono::duration<float> getOrdering() const;
    std::chrono::duration<float> getCriticalPathDuration() const;
#endif

    void notify();
    virtual void exec() = 0;

    virtual bool isDone() const = 0;

    void incRefs();
    void decRefs();
    bool canFree() const;

    GarbageCollector<ChunkBase>::Registration &getGcRegistration() {
        return gcReg;
    }
    const GarbageCollector<ChunkBase>::Registration &getGcRegistration() const {
        return gcReg;
    }

protected:
    DataSeriesBase *ds;

    GarbageCollector<ChunkBase>::Registration gcReg;

    // This is actually the number of strong refs (ptrs) we have to it.
    // Additionally, there is a raw pointer in the DataSeries, and raw pointers in dependencies' dependents array.
    // DataSeries.destroyChunk manages freeing chunks.
#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<unsigned int> refs = 0;
#else
    unsigned int refs = 0;
#endif

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<unsigned int> notifies = 0;
    util::SpinLock mutex;
#endif
    std::vector<ChunkBase *> dependents;

    mutable std::chrono::duration<float> followingDuration;

#if ENABLE_CHUNK_DEBUG
    static std::string getIndentation(signed int inc) {
        static thread_local signed int ind = 0;
        ind += inc;
        assert(ind >= 0);
        return std::string(std::min(ind, ind - inc), ' ');
    }
#endif

    void updateMemoryUsage(std::make_signed<std::size_t>::type inc);
};

}

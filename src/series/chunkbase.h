#pragma once

#include <chrono>
#include <cmath>
#include <vector>
#include <atomic>
#include <string>

#include "jw_util/thread.h"

#include "series/chunkptrbase.h"

#include "defs/ENABLE_CHUNK_NAMES.h"
#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

#if ENABLE_CHUNK_MULTITHREADING
#include "util/spinlock.h"
#endif

namespace series { class DataSeriesBase; }

namespace series {

class ChunkBase {
public:
    ChunkBase(DataSeriesBase *ds);
    virtual ~ChunkBase();

#if ENABLE_CHUNK_NAMES
    void setName(std::string newName) {
        name = std::move(newName);
    }
    const std::string &getName() const {
        return name;
    }
#endif

    void addDependent(ChunkPtrBase dep);
    void removeDependent(ChunkBase *chunk);

#if ENABLE_CHUNK_MULTITHREADING
    std::chrono::duration<float> getOrdering() const;
    std::chrono::duration<float> getCriticalPathDuration() const;
#endif

    void notify();
    virtual void exec() = 0;

    virtual bool isDone() const = 0;

    void recordAccess();
    unsigned int getLastAccess() const;

    void incRefs();
    void decRefs();

    DataSeriesBase *getDataSeries() const {
        return ds;
    }

protected:
    DataSeriesBase *ds;

    std::atomic<unsigned int> lastAccessTime;

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<unsigned int> refs = 0;
#else
    unsigned int refs = 0;
#endif

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<unsigned int> notifies = 0;
    util::SpinLock mutex;
#endif
    std::vector<ChunkPtrBase> dependents;

    mutable std::chrono::duration<float> followingDuration;

#if ENABLE_CHUNK_NAMES
    static std::string getIndentation(signed int inc) {
        static thread_local signed int ind = 0;
        ind += inc;
        assert(ind >= 0);
        return std::string(std::min(ind, ind - inc), ' ');
    }

    std::string name;
#endif

    void updateMemoryUsage(std::make_signed<std::size_t>::type inc);
};

}

#pragma once

#include <vector>

#include "jw_util/thread.h"

#include "app/appcontext.h"
#include "util/taskscheduler.h"
#include "util/spinlock.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"
#define CHUNK_SIZE (static_cast<std::size_t>(1) << CHUNK_SIZE_LOG2)

namespace {

template <typename Type>
struct ChunkAllocator {
    typedef Type value_type;

    ChunkAllocator() {}
    template <typename Other> ChunkAllocator(const ChunkAllocator<Other> &other) {
        (void) other;
    };

    Type *allocate(std::size_t n) {
        return static_cast<Type *>(::operator new(n * sizeof(Type)));
    }
    void deallocate(Type *ptr, std::size_t n) {
        ::operator delete(ptr);
    }
};

template <typename T1, typename T2>
bool operator==(const ChunkAllocator<T1> &a, const ChunkAllocator<T2> &b) {
    (void) a;
    (void) b;
    return true;
}

template <typename T1, typename T2>
bool operator!=(const ChunkAllocator<T1> &a, const ChunkAllocator<T2> &b) {
    return !(a == b);
}

template <typename ValueType, typename OpType>
ValueType atomicApply(std::atomic<ValueType> &atom, OpType op) {
    ValueType prev = atom;
    while (!atom.compare_exchange_weak(prev, op(prev))) {}
    return prev;
}

}

namespace series {

class DataSeriesBase {
    friend class ChunkBase;

public:
    DataSeriesBase(app::AppContext &context)
        : context(context)
        , avgRunDuration(std::chrono::duration<float>::zero())
    {}

protected:
    app::AppContext &context;

    void recordDuration(std::chrono::duration<float> duration) {
        atomicApply(avgRunDuration, [duration](std::chrono::duration<float> ard) {
            static constexpr float durationSampleResponse = 0.1f;
            ard *= 1.0f - durationSampleResponse;
            ard += duration * durationSampleResponse;
            return ard;
        });
    }

private:
    std::atomic<std::chrono::duration<float>> avgRunDuration;
};

class ChunkBase {
public:
    ChunkBase(DataSeriesBase *ds)
        : ds(ds)
        , followingDuration(NAN)
    {
        jw_util::Thread::assert_main_thread();
    }

    void addDependency(ChunkBase *dependency) {
        std::lock_guard<util::SpinLock> lock(dependency->mutex);
        dependency->dependents.push_back(this);
    }

    unsigned int getComputedCount() const {
        return computedCount;
    }

    std::chrono::duration<float> getOrdering() const {
        return getCriticalPathDuration();
    }

    std::chrono::duration<float> getCriticalPathDuration() const {
        if (std::isnan(followingDuration.count())) {
            followingDuration = std::chrono::duration<float>::zero();
            for (ChunkBase *dep : dependents) {
                std::chrono::duration<float> depDur = dep->getCriticalPathDuration();
                if (depDur > followingDuration) {
                    followingDuration = depDur;
                }
            }
        }
        return ds->avgRunDuration.load() + followingDuration;
    }

    void exec() {
        assert(computedCount < CHUNK_SIZE);

        unsigned int prevNotifies = notifies;

        auto t1 = std::chrono::high_resolution_clock::now();
        unsigned int prevCount = computedCount;
        unsigned int count = computer(prevCount);
        auto t2 = std::chrono::high_resolution_clock::now();

        // Set this first so the dependents we notify know what we've computed.
        assert(count >= prevCount);
        computedCount = count;

        if (notifies.exchange(0) == prevNotifies) {
            // No notifies came in while we were computing.
            // Notifies is zero. The next notify() will relaunch exec().

            std::lock_guard<util::SpinLock> lock(mutex);
            for (ChunkBase *chunk : dependents) {
                chunk->notify();
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

protected:
    void setComputer(std::function<unsigned int (unsigned int computedCount)> &&func) {
        computer = std::move(func);
    }

    void notify() {
        if (notifies++ == 0) {
            static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
            if (ds->avgRunDuration.load() > taskLengthThreshold) {
                ds->context.get<util::TaskScheduler<ChunkBase>>().addTask(this);
            } else {
                exec();
            }
        }
    }

    DataSeriesBase *ds;

    std::atomic<unsigned int> computedCount = 0;
    std::atomic<unsigned int> notifies = 0;
    util::SpinLock mutex;
    std::vector<ChunkBase *> dependents;

    mutable std::chrono::duration<float> followingDuration;

    std::function<unsigned int (unsigned int)> computer;
};

extern thread_local ChunkBase *activeChunk;

template <typename ElementType>
class DataSeries : private DataSeriesBase {
public:
    class Chunk : public ChunkBase {
        friend class DataSeries<ElementType>;

    public:
        Chunk(DataSeries<ElementType> *ds, std::size_t chunkIndex)
            : ChunkBase(ds)
        {
            ChunkBase *prevActiveChunk = activeChunk;
            activeChunk = this;

            setComputer(ds->getChunkGenerator(chunkIndex, data));

            assert(activeChunk == this);
            activeChunk = prevActiveChunk;

            notify();
        }

        ElementType *getVolatileData() {
            return data;
        }

        ElementType getElement(unsigned int index) {
            assert(index < computedCount);
            return data[index];
        }

    private:
        ElementType data[CHUNK_SIZE];
    };

#if 0
    typedef std::shared_ptr<Chunk> ChunkPtr;
    ChunkPtr constructChunk(std::size_t index) {
        return std::make_shared<Chunk>(this, index);
    }
#else
    typedef Chunk *ChunkPtr;
    ChunkPtr constructChunk(std::size_t index) {
        return context.get<util::Pool<Chunk>>().alloc(this, index);
    }
#endif


    DataSeries(app::AppContext &context)
        : DataSeriesBase(context)
    {
        jw_util::Thread::set_main_thread();
    }

    ChunkPtr getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        if (chunks.size() <= chunkIndex) {
            chunks.resize(chunkIndex + 1, 0);
        }
        if (!chunks[chunkIndex]) {
            chunks[chunkIndex] = constructChunk(chunkIndex);
        }

        if (activeChunk != 0) {
            activeChunk->addDependency(chunks[chunkIndex]);
        }

        return chunks[chunkIndex];
    }

    virtual std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) = 0;

private:
    std::vector<ChunkPtr> chunks;
};

}

#pragma once

#include <vector>

#include "function2/include/function2/function2.hpp"

#include "jw_util/thread.h"

#include "app/appcontext.h"
#include "util/taskscheduler.h"
#include "util/spinlock.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"
static_assert(CHUNK_SIZE_LOG2 < sizeof(unsigned int) * CHAR_BIT, "Lots of things depend on an inter-chunk index fitting into an unsigned int");
#define CHUNK_SIZE (static_cast<unsigned int>(1) << CHUNK_SIZE_LOG2)

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
        (void) n;

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

class ChunkBase;

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

    virtual void destroyChunk(ChunkBase *chunk) = 0;

private:
    std::atomic<std::chrono::duration<float>> avgRunDuration;
};

class ChunkBase;

class ChunkPtrBase {
public:
    ~ChunkPtrBase();

    ChunkPtrBase(const ChunkPtrBase &) = delete;
    ChunkPtrBase &operator=(const ChunkPtrBase &) = delete;

    ChunkPtrBase(ChunkPtrBase &&other)
        : target(std::move(other.target))
    {
        other.target = 0;
    }

    ChunkPtrBase &operator=(ChunkPtrBase &&other) {
        target = std::move(other.target);
        other.target = 0;
        return *this;
    }

    ChunkBase *operator->() const {
        assert(has());
        return target;
    }

    bool has() const {
        return target;
    }

    ChunkPtrBase clone() const {
        return ChunkPtrBase(operator->());
    }

    static ChunkPtrBase null() {
        return ChunkPtrBase(nullptr);
    }

protected:
    ChunkPtrBase(ChunkBase *ptr);

    ChunkPtrBase(std::nullptr_t)
        : target(0)
    {}

private:
    ChunkBase *target;
};

class ChunkBase {
public:
    ChunkBase(DataSeriesBase *ds)
        : ds(ds)
        , followingDuration(NAN)
    {
        jw_util::Thread::assert_main_thread();
    }

    ~ChunkBase() {
        assert(notifies == 0);
    }

    void addDependent(ChunkPtrBase dep) {
        std::lock_guard<util::SpinLock> lock(mutex);
        dependents.push_back(std::move(dep));
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
            for (const ChunkPtrBase &dep : dependents) {
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

        if (notifies.exchange(0) == prevNotifies || count == CHUNK_SIZE) {
            if (count == CHUNK_SIZE) {
                // Destroy any smart_ptrs that the lambda had captured.
                // This allows dependency chunks to be destroyed.
                computer = fu2::unique_function<unsigned int (unsigned int)>();
            }

            // No notifies came in while we were computing.
            // Notifies is zero. The next notify() will relaunch exec().

            if (count != prevCount) {
                std::lock_guard<util::SpinLock> lock(mutex);
                for (const ChunkPtrBase &dep : dependents) {
                    dep->notify();
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

    void notify() {
        if (notifies++ == 0) {
            if (computedCount == CHUNK_SIZE) {
                notifies--;
            } else {
                static constexpr std::chrono::duration<float> taskLengthThreshold = std::chrono::microseconds(20);
                if (ds->avgRunDuration.load() > taskLengthThreshold) {
                    ds->context.get<util::TaskScheduler<ChunkBase>>().addTask(this);
                } else {
                    exec();
                }
            }
        }
    }

    void incRefs() {
        refs++;
    }
    void decRefs() {
        if (--refs == 0) {
            ds->destroyChunk(this);
        }
    }

protected:
    void setComputer(fu2::unique_function<unsigned int (unsigned int)> &&func) {
        computer = std::move(func);
    }

    DataSeriesBase *ds;

    std::atomic<unsigned int> refs = 0;

    std::atomic<unsigned int> computedCount = 0;
    std::atomic<unsigned int> notifies = 0;
    util::SpinLock mutex;
    std::vector<ChunkPtrBase> dependents;

    mutable std::chrono::duration<float> followingDuration;

    fu2::unique_function<unsigned int (unsigned int)> computer;
};

extern thread_local ChunkPtrBase activeChunk;

template <typename ElementType>
class DataSeries : private DataSeriesBase {
public:
    class Chunk : public ChunkBase {
        friend class DataSeries<ElementType>;

    public:
        Chunk(DataSeries<ElementType> *ds, std::size_t chunkIndex)
            : ChunkBase(ds)
        {
            ChunkPtrBase prevActiveChunk = std::move(activeChunk);
            activeChunk = ChunkPtr::construct(this);

            setComputer(ds->getChunkGenerator(chunkIndex, data));

            assert(activeChunk.operator->() == this);
            activeChunk = std::move(prevActiveChunk);

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
#elif 0
    typedef Chunk *ChunkPtr;
    ChunkPtr constructChunk(std::size_t index) {
        return context.get<util::Pool<Chunk>>().alloc(this, index);
    }
#elif 1
    class ChunkPtr : public ChunkPtrBase {
    public:
        Chunk *operator->() const {
            assert(has());
            return static_cast<Chunk *>(ChunkPtrBase::operator->());
        }

        static ChunkPtr construct(Chunk *ptr) {
            return ChunkPtr(ptr);
        }

        ChunkPtr clone() const {
            return ChunkPtr(operator->());
        }

        static ChunkPtr null() {
            return ChunkPtr(nullptr);
        }

    protected:
        ChunkPtr(Chunk *ptr)
            : ChunkPtrBase(ptr)
        {
            assert(ptr);
        }

        ChunkPtr(std::nullptr_t)
            : ChunkPtrBase(nullptr)
        {}
    };

    ChunkPtr createChunk(std::size_t index) {
        return ChunkPtr::construct(context.get<util::Pool<Chunk>>().alloc(this, index));
    }
    void destroyChunk(ChunkBase *chunk) override {
        context.get<util::Pool<Chunk>>().free(static_cast<Chunk *>(chunk));
    }
#endif


    DataSeries(app::AppContext &context)
        : DataSeriesBase(context)
    {
        jw_util::Thread::set_main_thread();
    }

    ChunkPtr getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        while (chunks.size() <= chunkIndex) {
            chunks.emplace_back(ChunkPtr::null());
        }
        if (!chunks[chunkIndex].has()) {
            chunks[chunkIndex] = createChunk(chunkIndex);
        }

        if (activeChunk.has()) {
            chunks[chunkIndex]->addDependent(activeChunk.clone());
        }

        return chunks[chunkIndex].clone();
    }

    virtual fu2::unique_function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) = 0;

private:
    std::vector<ChunkPtr> chunks;
};

}

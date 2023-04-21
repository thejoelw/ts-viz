#pragma once

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"
#include "defs/ENABLE_NOTIFICATION_TRACING.h"

#if ENABLE_CHUNK_MULTITHREADING
#include <mutex>
#endif

#if ENABLE_NOTIFICATION_TRACING
#include "log.h"
#ifdef NDEBUG
    static_assert(false, "Should not have ENABLE_NOTIFICATION_TRACING enabled in release variant!");
#endif
#endif

#include "series/chunkbase.h"
#include "series/chunkptr.h"
#include "series/dataseriesbase.h"

namespace series {

extern thread_local ChunkPtrBase activeChunk;

template <typename ElementType, std::size_t _size = CHUNK_SIZE>
class Chunk : public ChunkBase {
public:
    static constexpr std::size_t size = _size;

    Chunk(DataSeriesBase *ds)
        : ChunkBase(ds)
    {}

    unsigned int getComputedCount() const {
        return computedCount;
    }

    void exec() override {
        assert(computedCount < size);

#if ENABLE_CHUNK_MULTITHREADING
        unsigned int prevNotifies = notifies;
        auto t1 = std::chrono::high_resolution_clock::now();
#endif
        unsigned int prevCount = computedCount;
        unsigned int count = compute(data, prevCount);
        assert(count >= prevCount);
        assert(count <= size);
#if ENABLE_CHUNK_MULTITHREADING
        auto t2 = std::chrono::high_resolution_clock::now();
#endif

        // Set this first so the dependents we notify know what we've computed.
        computedCount = count;

#if ENABLE_NOTIFICATION_TRACING
        SPDLOG_TRACE(getIndentation(0) + "count: {} -> {}", prevCount, count);
#endif

        if (
#if ENABLE_CHUNK_MULTITHREADING
                notifies.exchange(0) == prevNotifies || count == size
#else
                true
#endif
        ) {
            // No notifies came in while we were computing.
            // Notifies is zero. The next notify() will relaunch exec().

            if (count != prevCount) {
                if (count == size) {
                    // Destroy any smart_ptrs that the lambda had captured.
                    // This allows dependency chunks to be destroyed.

                    releaseComputer();
                }

#if ENABLE_NOTIFICATION_TRACING
                SPDLOG_TRACE(getIndentation(2) + "notifying dependents: {{");
#endif

#if ENABLE_CHUNK_MULTITHREADING
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
#else
                for (ChunkPtrBase &dep : dependents) {
                    dep->notify();
                }
#endif

#if ENABLE_NOTIFICATION_TRACING
                SPDLOG_TRACE(getIndentation(-2) + "}} // notifying dependents");
#endif
            }
        } else {
            // One or more notifies came in while we were computing.
            // Either this call will re-launch exec(), or something came already and exec() is already running.
            notify();
        }

#if ENABLE_CHUNK_MULTITHREADING
        ds->recordDuration(std::chrono::duration(t2 - t1) / std::max(1u, count - prevCount));
#endif
    }

    bool isDone() const override {
        assert(computedCount <= size);
        return computedCount == size;
    }

    const ElementType *getData() const {
        return data;
    }
    ElementType *getMutableData() {
        return data;
    }

    ElementType getElement(unsigned int index) const {
        assert(index < computedCount);
        return data[index];
    }

protected:
    virtual unsigned int compute(ElementType *dst, unsigned int computedCount) = 0;

    virtual void releaseComputer() = 0;

private:
#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<unsigned int> computedCount = 0;
#else
    unsigned int computedCount = 0;
#endif

    // Align to 16-byte boundary for fftw
    alignas(16) ElementType data[size];
};

}

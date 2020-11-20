#pragma once

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

        unsigned int prevNotifies = notifies;

        auto t1 = std::chrono::high_resolution_clock::now();
        unsigned int prevCount = computedCount;
        unsigned int count = compute(data, prevCount);
        assert(count >= prevCount);
        assert(count <= size);
        auto t2 = std::chrono::high_resolution_clock::now();

        // Set this first so the dependents we notify know what we've computed.
        computedCount = count;

        if (notifies.exchange(0) == prevNotifies || count == size) {
            // No notifies came in while we were computing.
            // Notifies is zero. The next notify() will relaunch exec().

            if (count != prevCount) {
                if (count == size) {
                    // Destroy any smart_ptrs that the lambda had captured.
                    // This allows dependency chunks to be destroyed.

                    releaseComputer();
                }

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

    ElementType *getVolatileData() {
        return data;
    }

    ElementType getElement(unsigned int index) {
        assert(index < computedCount);
        return data[index];
    }

protected:
    virtual unsigned int compute(ElementType *dst, unsigned int computedCount) const = 0;

    virtual void releaseComputer() = 0;

private:
    std::atomic<unsigned int> computedCount = 0;

    // Align to 16-byte boundary for fftw
    alignas(16) ElementType data[size];
};

}

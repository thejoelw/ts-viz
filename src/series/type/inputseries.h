#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

// TODO: Hold chunk pointer (prevents refcount thrashing)

namespace series {

template <typename ElementType>
class InputSeries : public DataSeries<ElementType> {
public:
    InputSeries(app::AppContext &context, const std::string &name)
        : DataSeries<ElementType>(context, false)
    {
        (void) name;
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        return this->constructChunk([this, chunkIndex](ElementType *dst, unsigned int computedCount) -> unsigned int {
            (void) dst;
            (void) computedCount;

            std::uint64_t ni = nextIndex;
            std::size_t finishedChunks = ni / CHUNK_SIZE;
            if (chunkIndex < finishedChunks) {
                return CHUNK_SIZE;
            } else if (chunkIndex > finishedChunks) {
                return 0;
            } else {
                return ni % CHUNK_SIZE;
            }
        });
    }

    void propagateUntil(std::uint64_t index) {
        if (index == notifyIdx) {
            return;
        }

        propagateUntilImpl(index);

        std::size_t end = (index - 1) / CHUNK_SIZE;
        for (std::size_t i = notifyIdx / CHUNK_SIZE; i <= end; i++) {
            this->getChunk(i)->notify();
        }

        notifyIdx = index;
    }

    void set(std::uint64_t index, ElementType value) {
        propagateUntilImpl(index);

        prevValue = value;
        this->getChunk(index / CHUNK_SIZE)->getMutableData()[index % CHUNK_SIZE] = value;
        nextIndex++;
    }

private:
    ElementType prevValue = NAN;

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::uint64_t> nextIndex = 0;
#else
    std::uint64_t nextIndex = 0;
#endif
    std::size_t notifyIdx = 0;

    void propagateUntilImpl(std::uint64_t index) {
        assert(nextIndex <= index);
        while (nextIndex < index) {
            this->getChunk(nextIndex / CHUNK_SIZE)->getMutableData()[nextIndex % CHUNK_SIZE] = prevValue;
            nextIndex++;
        }
        assert(nextIndex == index);
    }
};

}

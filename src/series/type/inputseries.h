#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

namespace series {

template <typename ElementType>
class InputSeries : public DataSeries<ElementType> {
public:
    InputSeries(app::AppContext &context, const std::string &name)
        : DataSeries<ElementType>(context)
    {
        (void) name;
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        return this->constructChunk([this, chunkIndex](ElementType *dst, unsigned int computedCount) -> unsigned int {
            (void) dst;
            (void) computedCount;

            std::size_t finishedChunks = nextIndex / CHUNK_SIZE;
            if (chunkIndex < finishedChunks) {
                return CHUNK_SIZE;
            } else if (chunkIndex > finishedChunks) {
                return 0;
            } else {
                return nextIndex % CHUNK_SIZE;
            }
        });
    }

    void propagateUntil(std::size_t index) {
        wrapNotify([this, index]() {
            propagateUntilImpl(index);
            assert(nextIndex == index);
        });
    }

    void set(std::size_t index, ElementType value) {
        wrapNotify([this, index, value]() {
            propagateUntilImpl(index);
            assert(nextIndex == index);

            prevValue = value;
            this->getChunk(index / CHUNK_SIZE)->getMutableData()[index % CHUNK_SIZE] = value;

            nextIndex++;
        });
    }

private:
    ElementType prevValue = NAN;

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::size_t> nextIndex = 0;
#else
    std::size_t nextIndex = 0;
#endif

    void propagateUntilImpl(std::size_t index) {
        assert(nextIndex <= index);
        while (nextIndex < index) {
            this->getChunk(nextIndex / CHUNK_SIZE)->getMutableData()[nextIndex % CHUNK_SIZE] = prevValue;
            nextIndex++;
        }
        assert(nextIndex == index);
    }

    template <typename FuncType>
    void wrapNotify(FuncType func) {
        std::size_t prevChunk = nextIndex / CHUNK_SIZE;

        func();

        while (prevChunk <= (nextIndex - 1) / CHUNK_SIZE) {
            this->getChunk(prevChunk)->notify();
            prevChunk++;
        }
    }
};

}

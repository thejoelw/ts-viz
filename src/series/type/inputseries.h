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

    void set(std::size_t index, ElementType value) {
        std::size_t ctr = nextIndex;
        std::size_t prevChunk = ctr / CHUNK_SIZE;

        assert(index >= ctr);
        while (ctr < index) {
            this->getChunk(ctr / CHUNK_SIZE)->getMutableData()[ctr % CHUNK_SIZE] = prevValue;
            ctr++;
        }

        prevValue = value;
        this->getChunk(ctr / CHUNK_SIZE)->getMutableData()[ctr % CHUNK_SIZE] = value;

        nextIndex = ctr + 1;

        while (prevChunk <= ctr / CHUNK_SIZE) {
            this->getChunk(prevChunk)->notify();
            prevChunk++;
        }
    }

private:
    ElementType prevValue = NAN;

#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::size_t> nextIndex = 0;
#else
    std::size_t nextIndex = 0;
#endif
};

}

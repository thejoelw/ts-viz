#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType>
class InputSeries : public DataSeries<ElementType> {
public:
    InputSeries(app::AppContext &context, const std::string &name)
        : DataSeries<ElementType>(context)
    {
        (void) name;
    }

    std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        (void) dst;

        return [this, chunkIndex](unsigned int computedCount) {
            (void) computedCount;

            std::size_t finishedChunks = nextIndex / CHUNK_SIZE;
            if (chunkIndex < finishedChunks) {
                return CHUNK_SIZE;
            } else if (chunkIndex > finishedChunks) {
                return 0;
            } else {
                return nextIndex % CHUNK_SIZE;
            }
        };
    }

    void set(std::size_t index, ElementType value) {
        assert(index >= nextIndex);
        while  (nextIndex < index) {
            this->getChunk(nextIndex / CHUNK_SIZE)->getVolatileData()[nextIndex % CHUNK_SIZE] = prevValue;
            nextIndex++;
        }

        prevValue = value;
        this->getChunk(nextIndex / CHUNK_SIZE)->getVolatileData()[nextIndex % CHUNK_SIZE] = value;
        nextIndex++;
    }

private:
    ElementType prevValue = NAN;
    std::atomic<std::size_t> nextIndex = 0;
};

}

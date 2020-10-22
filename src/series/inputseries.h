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

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        (void) chunkIndex;

        activeTask->addDependency();

        return [](ElementType *dst) {
            (void) dst;
        };
    }

    void set(std::size_t index, ElementType value) {
        std::size_t prevChunk = nextIndex / CHUNK_SIZE;

        assert(index >= nextIndex);
        while  (nextIndex < index) {
            this->getChunk(nextIndex / CHUNK_SIZE)->getVolatileData()[nextIndex % CHUNK_SIZE] = prevValue;
            nextIndex++;
        }

        prevValue = value;
        this->getChunk(nextIndex / CHUNK_SIZE)->getVolatileData()[nextIndex % CHUNK_SIZE] = value;
        nextIndex++;

        while (prevChunk < nextIndex / CHUNK_SIZE) {
            this->getChunk(prevChunk)->getTask().finishDependency(this->context.template get<util::TaskScheduler>());
            prevChunk++;
        }
    }

private:
    ElementType prevValue = NAN;
    std::size_t nextIndex = 0;
};

}

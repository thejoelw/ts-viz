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
        activeTask->addDependency();

        return [](ElementType *dst) {
            (void) dst;
        };
    }

    void set(std::size_t index, ElementType value) {
        static constexpr std::size_t size = DataSeries<ElementType>::Chunk::size;

        std::size_t prevChunk = nextIndex / size;

        assert(index >= nextIndex);
        while  (nextIndex < index) {
            this->getChunk(nextIndex / size)->getVolatileData()[nextIndex % size] = prevValue;
            nextIndex++;
        }

        prevValue = value;
        this->getChunk(nextIndex / size)->getVolatileData()[nextIndex % size] = value;
        nextIndex++;

        while (prevChunk < nextIndex / size) {
            this->getChunk(prevChunk)->getTask().finishDependency(this->context.template get<util::TaskScheduler>());
            prevChunk++;
        }
    }

private:
    ElementType prevValue = NAN;
    std::size_t nextIndex = 0;
};

}

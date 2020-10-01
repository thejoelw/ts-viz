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

    std::string getName() const override { return "input"; }

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        this->activeComputingChunk->getTask().addDependency();

        return [](ElementType *dst) {
            (void) dst;
        };
    }

    void set(std::size_t index, ElementType value) {
        static constexpr std::size_t size = DataSeries<ElementType>::Chunk::size;
//        this->modifyChunk(index / size)[index % size] = value;
        this->getChunk(index / size)->getVolatileData()[index % size] = value;
        if (index % size == size - 1) {
            this->getChunk(index / size)->getTask().finishDependency(this->context.template get<util::TaskScheduler>());
        }
    }
};

}

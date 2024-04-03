#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"
#include "stream/drawingmanager.h"

namespace series {

template <typename ElementType, typename TriggerType>
class DrawnSeries : public DataSeries<ElementType> {
public:
    DrawnSeries(app::AppContext &context, const std::string &name, TriggerType trigger)
        : DataSeries<ElementType>(context, false)
        , trigger(trigger)
        , drawing(context.get<stream::DrawingManager>().getStream(name))
    {
        (void) name;
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        auto triggerChunk = trigger.getChunk(chunkIndex);
        return this->constructChunk([this, chunkIndex, triggerChunk = std::move(triggerChunk)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            unsigned int endCount = triggerChunk->getComputedCount();
            unsigned int offset = chunkIndex * CHUNK_SIZE;
            for (std::size_t i = computedCount; i < endCount; i++) {
                dst[i] = drawing.sample(offset + i);
            }
            return endCount;
        });
    }

private:
    TriggerType trigger;
    stream::Drawing &drawing;
};

}

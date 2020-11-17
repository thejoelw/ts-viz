#pragma once

#include "series/dataseries.h"

namespace series {

template <typename ElementType>
class DelayedSeries : public DataSeries<ElementType> {
public:
    DelayedSeries(app::AppContext &context, DataSeries<ElementType> &arg, std::size_t delay)
        : DataSeries<ElementType>(context)
        , arg(arg)
        , delay(delay)
    {}

    std::function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) override {
        typedef typename series::DataSeries<ElementType>::ChunkPtr ChunkPtr;

        std::size_t d0 = (delay + CHUNK_SIZE - 1) / CHUNK_SIZE;
        std::size_t d1 = delay / CHUNK_SIZE;
        ChunkPtr c0 = d0 <= chunkIndex ? arg.getChunk(chunkIndex - d0) : ChunkPtr(0);
        ChunkPtr c1 = d1 <= chunkIndex ? arg.getChunk(chunkIndex - d1) : ChunkPtr(0);

        return [this, dst, c0 = std::move(c0), c1 = std::move(c1)](unsigned int computedCount) -> unsigned int {
            // TODO
            /*
            unsigned int delayMod = delay % CHUNK_SIZE;
            signed int index = computedCount;

            signed int count = c0->getComputedCount() + delayMod - CHUNK_SIZE;
            while (index < count) {
                dst[index] = c0->getElement(index - delayMod);
                index++;
            }



            unsigned int count;
            if (c0->getComputedCount() == CHUNK_SIZE) {
                count = c1->getComputedCount() + delay % CHUNK_SIZE;
            } else {

            }
            dst = c0 ? std::copy_n(c0->getVolatileData() + (CHUNK_SIZE - delay), delay, dst) : std::fill_n(dst, delay, NAN);
            dst = c1 ? std::copy_n(c1->getVolatileData(), CHUNK_SIZE - delay, dst) : std::fill_n(dst, CHUNK_SIZE - delay, NAN);
            */
            return 0;
        };
    }

private:
    DataSeries<ElementType> &arg;

    std::size_t delay;
};

}

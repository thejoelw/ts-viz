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

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        typedef typename series::DataSeries<ElementType>::ChunkPtr ChunkPtr;

        std::size_t d0 = (delay + CHUNK_SIZE - 1) / CHUNK_SIZE;
        std::size_t d1 = delay / CHUNK_SIZE;
        ChunkPtr c0 = d0 <= chunkIndex ? arg.getChunk(chunkIndex - d0) : ChunkPtr(0);
        ChunkPtr c1 = d1 <= chunkIndex ? arg.getChunk(chunkIndex - d1) : ChunkPtr(0);

        return [this, c0 = std::move(c0), c1 = std::move(c1)](ElementType *dst) {
            dst = c0 ? std::copy_n(c0->getData() + (CHUNK_SIZE - delay), delay, dst) : std::fill_n(dst, delay, NAN);
            dst = c1 ? std::copy_n(c1->getData(), CHUNK_SIZE - delay, dst) : std::fill_n(dst, CHUNK_SIZE - delay, NAN);
        };
    }

private:
    DataSeries<ElementType> &arg;

    std::size_t delay;
};

}

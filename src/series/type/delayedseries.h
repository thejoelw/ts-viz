#pragma once

#include "series/dataseries.h"
#include "series/invalidparameterexception.h"

namespace series {

template <typename ElementType>
class DelayedSeries : public DataSeries<ElementType> {
public:
    DelayedSeries(app::AppContext &context, DataSeries<ElementType> &arg, std::int64_t delay)
        : DataSeries<ElementType>(context)
        , arg(arg)
        , delay(delay)
    {
        if (delay <= 0) {
            throw series::InvalidParameterException("DelayedSeries: delay must be greater than zero");
        }
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        std::size_t d0 = (delay + CHUNK_SIZE - 1) / CHUNK_SIZE;
        std::size_t d1 = delay / CHUNK_SIZE;
        auto c0 = d0 <= chunkIndex ? arg.getChunk(chunkIndex - d0) : ChunkPtr<ElementType>::null();
        auto c1 = d1 <= chunkIndex ? arg.getChunk(chunkIndex - d1) : ChunkPtr<ElementType>::null();

        return this->constructChunk([this, c0 = std::move(c0), c1 = std::move(c1)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            unsigned int delayMod = delay % CHUNK_SIZE;

            while (computedCount < CHUNK_SIZE) {
                if (computedCount < delayMod) {
                    if (c0.has()) {
                        unsigned int index = computedCount + CHUNK_SIZE - delayMod;
                        if (index < c0->getComputedCount()) {
                            dst[computedCount] = c0->getElement(index);
                        } else {
                            break;
                        }
                    } else {
                        dst[computedCount] = NAN;
                    }
                } else {
                    if (c1.has()) {
                        unsigned int index = computedCount - delayMod;
                        if (index < c1->getComputedCount()) {
                            dst[computedCount] = c1->getElement(index);
                        } else {
                            break;
                        }
                    } else {
                        dst[computedCount] = NAN;
                    }
                }
                computedCount++;
            }

            return computedCount;
        });
    }

private:
    DataSeries<ElementType> &arg;

    std::size_t delay;
};

}

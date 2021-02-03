#pragma once

#include "jw_util/hash.h"

#include "series/dataseries.h"

namespace series {

template <typename ElementType, typename OperatorType>
class CompSeries : public DataSeries<ElementType> {
public:
    CompSeries(app::AppContext &context, OperatorType op)
        : DataSeries<ElementType>(context)
        , op(op)
    {}

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        return this->constructChunk([this, chunkIndex](ElementType *dst, unsigned int computedCount) -> unsigned int {
            assert(computedCount == 0);

            for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
                dst[i] = op(static_cast<std::uint64_t>(chunkIndex) * CHUNK_SIZE + i);
            }

            return CHUNK_SIZE;
        });
    }

private:
    OperatorType op;
};

}

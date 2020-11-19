#pragma once

#include "series/base/chunkbase.h"
#include "series/base/chunksize.h"
#include "series/base/chunkptr.h"
#include "series/base/dataseriesbase.h"

namespace series {

extern thread_local ChunkPtrBase activeChunk;

template <typename ElementType>
class Chunk : public ChunkBase {
public:
    template <typename ComputerGenerator>
    Chunk(DataSeriesBase *ds, ComputerGenerator computerGenerator)
        : ChunkBase(ds)
    {
        ChunkPtrBase prevActiveChunk = std::move(activeChunk);
        activeChunk = ChunkPtr<ElementType>::construct(this);

        setComputer(computerGenerator(data));

        assert(activeChunk.operator->() == this);
        activeChunk = std::move(prevActiveChunk);

        notify();
    }

    ElementType *getVolatileData() {
        return data;
    }

    ElementType getElement(unsigned int index) {
        assert(index < computedCount);
        return data[index];
    }

private:
    ElementType data[CHUNK_SIZE];
};

}

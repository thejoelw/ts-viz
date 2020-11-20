#pragma once

#include "series/chunkptrbase.h"
#include "series/chunksize.h"

namespace series { template <typename ElementType, std::size_t dataSize> class Chunk; }

namespace series {

template <typename ElementType, std::size_t size = CHUNK_SIZE>
class ChunkPtr : public ChunkPtrBase {
public:
    Chunk<ElementType, size> *operator->() const {
        assert(has());
        return static_cast<Chunk<ElementType, size> *>(ChunkPtrBase::operator->());
    }

    static ChunkPtr construct(Chunk<ElementType, size> *ptr) {
        return ChunkPtr(ptr);
    }

    ChunkPtr clone() const {
        return ChunkPtr(operator->());
    }

    static ChunkPtr null() {
        return ChunkPtr(nullptr);
    }

protected:
    ChunkPtr(Chunk<ElementType, size> *ptr)
        : ChunkPtrBase(ptr)
    {
        assert(ptr);
        ptr->recordAccess();
    }

    ChunkPtr(std::nullptr_t)
        : ChunkPtrBase(nullptr)
    {}
};

}

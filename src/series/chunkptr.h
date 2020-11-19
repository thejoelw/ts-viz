#pragma once

#include "series/chunkptrbase.h"

namespace series { template <typename ElementType> class Chunk; }

namespace series {

template <typename ElementType>
class ChunkPtr : public ChunkPtrBase {
public:
    Chunk<ElementType> *operator->() const {
        assert(has());
        return static_cast<Chunk<ElementType> *>(ChunkPtrBase::operator->());
    }

    static ChunkPtr construct(Chunk<ElementType> *ptr) {
        return ChunkPtr(ptr);
    }

    ChunkPtr clone() const {
        return ChunkPtr(operator->());
    }

    static ChunkPtr null() {
        return ChunkPtr(nullptr);
    }

protected:
    ChunkPtr(Chunk<ElementType> *ptr)
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

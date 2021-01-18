#pragma once

#include <cassert>
#include <utility>

namespace series { class ChunkBase; }

namespace series {

class ChunkPtrBase {
public:
    ~ChunkPtrBase();

    ChunkPtrBase(const ChunkPtrBase &) = delete;
    ChunkPtrBase &operator=(const ChunkPtrBase &) = delete;

    ChunkPtrBase(ChunkPtrBase &&other)
        : target(std::move(other.target))
    {
        other.target = 0;
    }

    ChunkPtrBase &operator=(ChunkPtrBase &&other) {
        std::swap(target, other.target);
        return *this;
    }

    ChunkBase *operator->() const {
        assert(has());
        return target;
    }

    bool has() const {
        return target;
    }

    ChunkPtrBase clone() const {
        return ChunkPtrBase(operator->());
    }

    static ChunkPtrBase null() {
        return ChunkPtrBase(nullptr);
    }

protected:
    ChunkPtrBase(ChunkBase *ptr);

    ChunkPtrBase(std::nullptr_t)
        : target(0)
    {}

private:
    ChunkBase *target;
};

}

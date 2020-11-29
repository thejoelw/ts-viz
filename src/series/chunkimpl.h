#pragma once

#include "series/chunk.h"

namespace series {

template <typename ElementType, std::size_t size, typename ComputerType>
class ChunkImpl : public Chunk<ElementType, size> {
public:
    ChunkImpl(DataSeriesBase *ds, ComputerType &&computer)
        : Chunk<ElementType, size>(ds)
        , computer(std::move(computer))
    {}

    ~ChunkImpl() {
        if (!this->isDone()) {
            releaseComputer();
        }

#ifndef NDEBUG
        assert(!hasValue);
#endif
    }

    unsigned int compute(ElementType *dst, unsigned int computedCount) override {
#ifndef NDEBUG
        static constexpr std::size_t sizeofThis = sizeof(ChunkImpl<ElementType, size, ComputerType>);
        static constexpr std::size_t sizeofComputer = sizeof(ComputerType);
        assert(hasValue);
#endif

        return computer(dst, computedCount);
    }

    void releaseComputer() override {
#ifndef NDEBUG
        assert(hasValue);
        hasValue = false;
#endif

        computer.~ComputerType();
    }

private:
#ifndef NDEBUG
    bool hasValue = true;
#endif

    union {
        ComputerType computer;
        char _; // Prevent destruction of computer
    };
};

}

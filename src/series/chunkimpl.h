#pragma once

#include "series/chunk.h"
#include "log.h"

namespace series {

template <typename ElementType, std::size_t size, typename ComputerType>
class ChunkImpl final /* final because we want sizeof(this) to be correct */ : public Chunk<ElementType, size> {
public:
    ChunkImpl(DataSeriesBase *ds, ComputerType &&computer)
        : Chunk<ElementType, size>(ds)
        , computer(std::move(computer))
    {
#if ENABLE_CHUNK_NAMES
        SPDLOG_DEBUG("Creating chunk with name {} and size {}", this->name, sizeof(*this));
#else
        SPDLOG_DEBUG("Creating chunk with size {}", sizeof(*this));
#endif

        this->updateMemoryUsage(sizeof(*this));
    }

    ~ChunkImpl() {
#ifndef NDEBUG
        assert(!isRunning);
#endif

        if (!this->isDone()) {
            releaseComputer();
        }

#ifndef NDEBUG
        assert(!hasValue);
#endif

        this->updateMemoryUsage(-sizeof(*this));

#if ENABLE_CHUNK_NAMES
        SPDLOG_DEBUG("Destroying chunk with name {} and size {}", this->name, sizeof(*this));
#else
        SPDLOG_DEBUG("Destroying chunk with size {}", sizeof(*this));
#endif
    }

    unsigned int compute(ElementType *dst, unsigned int computedCount) override {
#ifndef NDEBUG
        static constexpr std::size_t sizeofThis = sizeof(ChunkImpl<ElementType, size, ComputerType>);
        static constexpr std::size_t sizeofComputer = sizeof(ComputerType);
        assert(hasValue);

        assert(!isRunning);
        isRunning = true;
#endif

        unsigned int res = computer(dst, computedCount);

#ifndef NDEBUG
        assert(isRunning);
        isRunning = false;
#endif

        return res;
    }

    void releaseComputer() override {
#ifndef NDEBUG
        assert(hasValue);
        hasValue = false;

        assert(!isRunning);
#endif

        computer.~ComputerType();
    }

private:
#ifndef NDEBUG
    bool hasValue = true;
    bool isRunning = false;
#endif

    union {
        ComputerType computer;
        char _; // Prevent destruction of computer
    };
};

}

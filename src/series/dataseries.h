#pragma once

#include <vector>
#include <iomanip>

#include "jw_util/thread.h"

#include "series/dataseries.decl.h"
#include "series/dataseriesbase.h"
#include "series/chunkptr.h"
#include "series/chunk.h"
#include "series/chunkimpl.h"
#include "series/garbagecollector.h"

namespace series {

template <typename _ElementType, std::size_t _size>
class DataSeries : public DataSeriesBase {
public:
    typedef _ElementType ElementType;
    static constexpr std::size_t size = _size;

    DataSeries(app::AppContext &context, bool isTransient = true)
        : DataSeriesBase(context, isTransient)
    {}

    ~DataSeries() {
        // This is being destructed when an exception is thrown from the constructor.
        // TODO: Figure out how to catch bad destructions.
        // assert(false);
    }

    template <std::size_t desiredSize = CHUNK_SIZE>
    ChunkPtr<ElementType, size> getChunk(std::size_t chunkIndex) {
        static_assert(size == desiredSize, "DataSeries chunk size doesn't match desired size");

        jw_util::Thread::assert_main_thread();

        if (dryConstruct) {
            if (chunks[chunkIndex]) {
                getDependencyStack().push_back(chunks[chunkIndex]);
            }
            return ChunkPtr<ElementType, size>::null();
        }

        while (chunks.size() <= chunkIndex) {
            chunks.emplace_back(nullptr);
        }
        if (!chunks[chunkIndex]) {
            std::size_t depStackSize = getDependencyStack().size();
            chunks[chunkIndex] = makeChunk(chunkIndex);

            assert(getDependencyStack().size() >= depStackSize);
            while (getDependencyStack().size() > depStackSize) {
                getDependencyStack().back()->addDependent(chunks[chunkIndex]);
                getDependencyStack().pop_back();
            }
            chunks[chunkIndex]->notify();
        }

        getDependencyStack().push_back(chunks[chunkIndex]);

        return ChunkPtr<ElementType, size>::construct(chunks[chunkIndex]);
    }

    void releaseChunk(const ChunkBase *chunk) {
        std::size_t chunkIndex = locateChunk(chunk);
        assert(chunk->canFree());

        jw_util::Thread::assert_main_thread();

        std::size_t depStackSize = getDependencyStack().size();

        assert(dryConstruct == false);
        dryConstruct = true;

        Chunk<ElementType, size> *dryChunk = makeChunk(chunkIndex);
        assert(dryChunk == nullptr);

        assert(dryConstruct == true);
        dryConstruct = false;

        assert(getDependencyStack().size() >= depStackSize);
        while (getDependencyStack().size() > depStackSize) {
            getDependencyStack().back()->removeDependent(chunk);
            getDependencyStack().pop_back();
        }

        SPDLOG_DEBUG("Nullify {}", static_cast<void *>(chunks[chunkIndex]));
        chunks[chunkIndex] = nullptr;
    }

#if ENABLE_CHUNK_DEBUG
    void writeDebug(std::ostream &dst) const {
        dst << std::setfill('0') << std::setw(16) << reinterpret_cast<std::uintptr_t>(this);
        bool first = true;
        for (const Meta &meta : metas) {
            if (first) {
                dst << ": ";
                first = false;
            } else {
                dst << ", ";
            }
            dst << meta.name << " from " << meta.trace;
        }
        dst << std::endl << "  ";

        for (const Chunk<ElementType, size> *chunk : chunks) {
            if (!chunk) {
                dst << '.';
            } else if (chunk->getGcRegistration().isEnqueued()) {
                dst << '-';
            } else {
                dst << '!';
            }
        }

        dst << std::endl;
    }
#endif

    virtual Chunk<ElementType, size> *makeChunk(std::size_t chunkIndex) = 0;

    const std::vector<Chunk<ElementType, size> *> &getChunks() const {
        return chunks;
    }

protected:
    template <typename ComputerType>
    Chunk<ElementType, size> *constructChunk(ComputerType &&computer) {
        if (dryConstruct) {
            return nullptr;
        } else {
            return new ChunkImpl<ElementType, size, ComputerType>(this, std::move(computer));
        }
    }

private:
//    std::uint64_t offset = 0;
    std::vector<Chunk<ElementType, size> *> chunks;

    std::size_t locateChunk(const ChunkBase *chunk) {
        // Search backwards because it's more likely that we're searching for a recent chunk.
        std::size_t idx = chunks.size();
        while (true) {
            idx--;
            if (chunks[idx] == chunk) {
                return idx;
            }

            // If this fails, the chunk doesn't exist here
            assert(idx > 0);
        }
    }
};

}

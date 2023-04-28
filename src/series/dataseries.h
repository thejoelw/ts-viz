#pragma once

#include <vector>

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
        : DataSeriesBase(context)
    {
        jw_util::Thread::set_main_thread();
        context.get<GarbageCollector>().registerDataSeries(this);
    }

    ~DataSeries() {
        // This is being destructed when an exception is thrown from the constructor.
        // TODO: Figure out how to catch bad destructions.
        // assert(false);

        context.get<GarbageCollector>().unregisterDataSeries(this);
    }

    template <std::size_t desiredSize = CHUNK_SIZE>
    ChunkPtr<ElementType, size> getChunk(std::size_t chunkIndex) {
        static_assert(size == desiredSize, "DataSeries chunk size doesn't match desired size");

        jw_util::Thread::assert_main_thread();

        if (dryConstruct) {
            getDependencyStack().push_back(chunks[chunkIndex].operator->());
            return ChunkPtr<ElementType, size>::null();
        }

        while (chunks.size() <= chunkIndex) {
            chunks.emplace_back(ChunkPtr<ElementType, size>::null());
        }
        if (!chunks[chunkIndex].has()) {
            std::size_t depStackSize = getDependencyStack().size();
            chunks[chunkIndex] = ChunkPtr<ElementType, size>::construct(makeChunk(chunkIndex));

#if ENABLE_CHUNK_NAMES
            chunks[chunkIndex]->setName(name + "[" + std::to_string(chunkIndex) + "]");
#endif

            assert(getDependencyStack().size() >= depStackSize);
            while (getDependencyStack().size() > depStackSize) {
                getDependencyStack().back()->addDependent(chunks[chunkIndex].clone());
                getDependencyStack().pop_back();
            }
            chunks[chunkIndex]->notify();
        }

        getDependencyStack().push_back(chunks[chunkIndex].operator->());

        return chunks[chunkIndex].clone();
    }

    void releaseChunk(std::size_t chunkIndex) {
        assert(chunkIndex < chunks.size());
        assert(chunks[chunkIndex].has());

        jw_util::Thread::assert_main_thread();

        std::size_t depStackSize = getDependencyStack().size();

        assert(dryConstruct == false);
        dryConstruct = true;

        Chunk<ElementType, size> *chunk = makeChunk(chunkIndex);
        assert(chunk == 0);

        assert(dryConstruct == true);
        dryConstruct = false;

        assert(getDependencyStack().size() >= depStackSize);
        while (getDependencyStack().size() > depStackSize) {
            getDependencyStack().back()->removeDependent(chunks[chunkIndex].operator->());
            getDependencyStack().pop_back();
        }

        chunks[chunkIndex] = ChunkPtr<ElementType, size>::null();
    }

    virtual Chunk<ElementType, size> *makeChunk(std::size_t chunkIndex) = 0;

    const std::vector<ChunkPtr<ElementType, size>> &getChunks() const {
        return chunks;
    }

    bool getIsTransient() const {
        return isTransient;
    }

protected:
    template <typename ComputerType>
    Chunk<ElementType, size> *constructChunk(ComputerType &&computer) {
        if (dryConstruct) {
            return 0;
        } else {
            return new ChunkImpl<ElementType, size, ComputerType>(this, std::move(computer));
        }
    }

private:
//    std::uint64_t offset = 0;
    std::vector<ChunkPtr<ElementType, size>> chunks;

    bool isTransient;
};

}

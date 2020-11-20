#pragma once

#include <vector>

#include "function2/include/function2/function2.hpp"

#include "jw_util/thread.h"

#include "series/dataseries.decl.h"
#include "series/dataseriesbase.h"
#include "series/chunkptr.h"
#include "series/chunk.h"
#include "series/chunkimpl.h"

namespace series {

template <typename ElementType, std::size_t size>
class DataSeries : public DataSeriesBase {
public:
    /*
    Chunk<ElementType> *createChunk(std::size_t index) {
        return new Chunk<ElementType>(this, [this, index](ElementType *data) {return makeChunkComputer(index, data);});
    }
    void destroyChunk(ChunkBase *chunk) override {
        delete static_cast<Chunk<ElementType> *>(chunk);
    }
    */

    DataSeries(app::AppContext &context)
        : DataSeriesBase(context)
    {
        jw_util::Thread::set_main_thread();
    }

    ChunkPtr<ElementType, size> getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        while (chunks.size() <= chunkIndex) {
            chunks.emplace_back(ChunkPtr<ElementType, size>::null());
        }
        if (!chunks[chunkIndex].has()) {
            std::size_t depStackSize = dependencyStack.size();
            chunks[chunkIndex] = ChunkPtr<ElementType, size>::construct(makeChunk(chunkIndex));
            while (dependencyStack.size() > depStackSize) {
                dependencyStack.back()->addDependent(chunks[chunkIndex].clone());
                dependencyStack.pop_back();
            }
            chunks[chunkIndex]->notify();
        }

        dependencyStack.push_back(chunks[chunkIndex].operator->());

        return chunks[chunkIndex].clone();
    }

    virtual Chunk<ElementType, size> *makeChunk(std::size_t chunkIndex) = 0;

protected:
    template <typename ComputerType>
    Chunk<ElementType, size> *constructChunk(ComputerType &&computer) {
        return new ChunkImpl<ElementType, size, ComputerType>(this, std::move(computer));
    }

private:
//    std::size_t offset = 0;
    std::vector<ChunkPtr<ElementType, size>> chunks;
};

}

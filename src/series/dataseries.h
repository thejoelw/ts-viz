#pragma once

#include <vector>

#include "function2/include/function2/function2.hpp"

#include "jw_util/thread.h"

#include "series/dataseriesbase.h"
#include "series/chunkptr.h"
#include "series/chunk.h"

namespace series {

template <typename ElementType>
class DataSeries : public DataSeriesBase {
public:
    Chunk<ElementType> *createChunk(std::size_t index) {
        return new Chunk<ElementType>(this, [this, index](ElementType *data) {return getChunkGenerator(index, data);});
    }
    void destroyChunk(ChunkBase *chunk) override {
        delete static_cast<Chunk<ElementType> *>(chunk);
    }

    DataSeries(app::AppContext &context)
        : DataSeriesBase(context)
    {
        jw_util::Thread::set_main_thread();
    }

    ChunkPtr<ElementType> getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        while (chunks.size() <= chunkIndex) {
            chunks.emplace_back(ChunkPtr<ElementType>::null());
        }
        if (!chunks[chunkIndex].has()) {
            std::size_t depStackSize = dependencyStack.size();
            chunks[chunkIndex] = ChunkPtr<ElementType>::construct(createChunk(chunkIndex));
            while (dependencyStack.size() > depStackSize) {
                dependencyStack.back()->addDependent(chunks[chunkIndex]);
                dependencyStack.pop_back();
            }
        }

        dependencyStack.push_back(chunks[chunkIndex]);

        return chunks[chunkIndex].clone();
    }

    virtual fu2::unique_function<unsigned int (unsigned int)> getChunkGenerator(std::size_t chunkIndex, ElementType *dst) = 0;

private:
//    std::size_t offset = 0;
    std::vector<ChunkPtr<ElementType>> chunks;
};

}

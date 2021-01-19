#pragma once

#include <queue>

#include "app/tickercontext.h"
#include "series/chunkbase.h"

namespace series {

class GarbageCollector : public app::TickerContext::TickableBase<GarbageCollector> {
private:
    struct ChunkIterator {
        ChunkIterator()
            : queue(&cmp)
        {}

        void operator()(ChunkPtrBase &ptr) {
            if (ptr->refCount() == 1) {
                queue.push(&ptr);
            }
        }

        static bool cmp(ChunkPtrBase *a, ChunkPtrBase *b) {
            return (*a)->getLastAccess() > (*b)->getLastAccess();
        }
        std::priority_queue<ChunkPtrBase *, std::vector<ChunkPtrBase *>, bool (*)(ChunkPtrBase *, ChunkPtrBase *)> queue;
    };

public:
    GarbageCollector(app::AppContext &context);

    void tick(app::TickerContext &tickerContext);

    template <typename DataSeriesType>
    void registerDataSeries(DataSeriesType *ds) {
        dsCollections.push_back(makeNeedle<DataSeriesType>(ds));
    }
    template <typename DataSeriesType>
    void unregisterDataSeries(DataSeriesType *ds) {
        std::vector<std::pair<void (*)(void *, ChunkIterator &), void *>>::iterator pos = std::find(dsCollections.begin(), dsCollections.end(), makeNeedle<DataSeriesType>(ds));
        assert(pos != dsCollections.end());

        // TODO: Make sure we're not iterating
        // If so, we need to zero it or something
        assert(false);

        *pos = dsCollections.back();
        dsCollections.pop_back();
    }

    void updateMemoryUsage(std::make_signed<std::size_t>::type inc);

    static unsigned int getCurrentTime() {
        return currentTime;
    }

private:
    inline static std::atomic<unsigned int> currentTime = 0;

    std::size_t memoryUsage = 0;

    std::vector<std::pair<void (*)(void *, ChunkIterator &), void *>> dsCollections;

    template <typename DataSeriesType>
    static std::pair<void (*)(void *, ChunkIterator &), void *> makeNeedle(DataSeriesType *ds) {
        return std::pair<void (*)(void *, ChunkIterator &), void *>(&callForeachChunk<DataSeriesType>, static_cast<void *>(ds));
    }

    template <typename DataSeriesType>
    static void callForeachChunk(void *ds, ChunkIterator &it) {
        static_cast<DataSeriesType *>(ds)->template foreachChunk<ChunkIterator &>(it);
    }
};

}

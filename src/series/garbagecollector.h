#pragma once

#include <queue>

#include "app/tickercontext.h"
#include "series/chunkbase.h"

// TODO: Remove
#include "log.h"
#include "series/dataseriesbase.h"

namespace series {

class GarbageCollector : public app::TickerContext::TickableBase<GarbageCollector> {
private:
    struct ChunkAddress {
        unsigned int lastAccess;
        void *ds;
        std::size_t index;
        void (*release)(void *ds, std::size_t index);

        bool operator<(const ChunkAddress &other) const {
            return lastAccess > other.lastAccess;
        }
    };
    typedef std::priority_queue<ChunkAddress> ChunkReleaseQueue;

public:
    GarbageCollector(app::AppContext &context);

    void tick(app::TickerContext &tickerContext);

    template <typename DataSeriesType>
    void registerDataSeries(DataSeriesType *ds) {
        dsCollections.push_back(makeNeedle<DataSeriesType>(ds));
    }
    template <typename DataSeriesType>
    void unregisterDataSeries(DataSeriesType *ds) {
        std::vector<std::pair<void (*)(void *, ChunkReleaseQueue &), void *>>::iterator pos = std::find(dsCollections.begin(), dsCollections.end(), makeNeedle<DataSeriesType>(ds));
        assert(pos != dsCollections.end());

        // TODO: Make sure we're not iterating
        // If so, we need to zero it or something
        assert(false);

        *pos = dsCollections.back();
        dsCollections.pop_back();
    }

    std::size_t getMemoryUsage() const {
        return memoryUsage;
    }
    void updateMemoryUsage(std::make_signed<std::size_t>::type inc);

    static unsigned int getCurrentTime() {
        return currentTime;
    }

private:
    inline static std::atomic<unsigned int> currentTime = 0;

    std::size_t memoryUsage = 0;

    std::vector<std::pair<void (*)(void *, ChunkReleaseQueue &), void *>> dsCollections;

    void runGc();

    template <typename DataSeriesType>
    static std::pair<void (*)(void *, ChunkReleaseQueue &), void *> makeNeedle(DataSeriesType *ds) {
        return std::pair<void (*)(void *, ChunkReleaseQueue &), void *>(&callForeachChunk<DataSeriesType>, static_cast<void *>(ds));
    }

    template <typename DataSeriesType>
    static void callForeachChunk(void *ds, ChunkReleaseQueue &queue) {
        const auto &chunks = static_cast<DataSeriesType *>(ds)->getChunks();
        for (std::size_t i = 0; i < chunks.size(); i++) {
            if (chunks[i].has()) {
                ChunkAddress addr;
                addr.lastAccess = chunks[i]->getLastAccess();
                addr.ds = ds;
                addr.index = i;
                addr.release = &chunkReleaser<DataSeriesType>;
                queue.push(addr);
            }
        }
    }

    template <typename DataSeriesType>
    static void chunkReleaser(void *ds, std::size_t index) {
        static_cast<DataSeriesType *>(ds)->releaseChunk(index);
    }
};

}

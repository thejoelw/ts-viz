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
        void *dsPtr;
        std::size_t index;
        void (*release)(void *dsPtr, std::size_t index);

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
        std::vector<std::pair<void (*)(void *, ChunkReleaseQueue &, bool), void *>>::iterator pos = std::find(dsCollections.begin(), dsCollections.end(), makeNeedle<DataSeriesType>(ds));
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

    std::vector<std::pair<void (*)(void *, ChunkReleaseQueue &, bool), void *>> dsCollections;

    void runGc();

    template <typename DataSeriesType>
    static std::pair<void (*)(void *, ChunkReleaseQueue &, bool), void *> makeNeedle(DataSeriesType *dsPtr) {
        return std::pair<void (*)(void *, ChunkReleaseQueue &, bool), void *>(&callForeachChunk<DataSeriesType>, static_cast<void *>(dsPtr));
    }

    template <typename DataSeriesType>
    static void callForeachChunk(void *dsPtr, ChunkReleaseQueue &queue, bool canReleaseInputs) {
        DataSeriesType *ds = static_cast<DataSeriesType *>(dsPtr);
        if (ds->getIsTransient() || canReleaseInputs) {
            const auto &chunks = ds->getChunks();
            for (std::size_t i = 0; i < chunks.size(); i++) {
                if (chunks[i].has()) {
                    ChunkAddress addr;
                    addr.lastAccess = chunks[i]->getLastAccess();
                    addr.dsPtr = dsPtr;
                    addr.index = i;
                    addr.release = &chunkReleaser<DataSeriesType>;
                    queue.push(addr);
                }
            }
        }
    }

    template <typename DataSeriesType>
    static void chunkReleaser(void *ds, std::size_t index) {
        static_cast<DataSeriesType *>(ds)->releaseChunk(index);
    }
};

}

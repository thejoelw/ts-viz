#pragma once

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"
#include "defs/ENABLE_CHUNK_NAMES.h"

#include <vector>
#if ENABLE_CHUNK_MULTITHREADING
#include <chrono>
#include <atomic>
#endif
#if ENABLE_CHUNK_NAMES
#include <string>
#endif

namespace app { class AppContext; }
namespace series { class ChunkBase; }

namespace series {

class DataSeriesBase {
public:
    DataSeriesBase(app::AppContext &context);

    virtual ~DataSeriesBase() {}

    app::AppContext &getContext() {
        return context;
    }

#if ENABLE_CHUNK_MULTITHREADING
    void recordDuration(std::chrono::duration<float> duration);
    std::chrono::duration<float> getAvgRunDuration() const;
#endif

#if ENABLE_CHUNK_NAMES
    void setName(std::string newName) {
        name = std::move(newName);
    }
    const std::string &getName() const {
        return name;
    }
#endif

    virtual void releaseChunk(const ChunkBase *chunk) = 0;

protected:
    app::AppContext &context;

    static std::vector<ChunkBase *> &getDependencyStack();

#if ENABLE_CHUNK_NAMES
    std::string name = "nameless";
#endif

    static thread_local bool dryConstruct;

private:
#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::chrono::duration<float>> avgRunDuration;
#endif

    static thread_local std::vector<ChunkBase *> dependencyStack;
};

}

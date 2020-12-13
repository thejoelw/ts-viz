#pragma once

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"

#include <vector>
#if ENABLE_CHUNK_MULTITHREADING
#include <chrono>
#include <atomic>
#endif

namespace app { class AppContext; }
namespace series { class ChunkBase; }

namespace series {

class DataSeriesBase {
public:
    DataSeriesBase(app::AppContext &context);

    app::AppContext &getContext() {
        return context;
    }

#if ENABLE_CHUNK_MULTITHREADING
    void recordDuration(std::chrono::duration<float> duration);
    std::chrono::duration<float> getAvgRunDuration() const;
#endif

protected:
    app::AppContext &context;

    static std::vector<ChunkBase *> &getDependencyStack();

private:
#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::chrono::duration<float>> avgRunDuration;
#endif

    static thread_local std::vector<ChunkBase *> dependencyStack;
};

}

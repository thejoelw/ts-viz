#pragma once

#include <chrono>
#include <atomic>
#include <vector>

namespace app { class AppContext; }
namespace series { class ChunkBase; }

namespace series {

class DataSeriesBase {
public:
    DataSeriesBase(app::AppContext &context);

    app::AppContext &getContext() {
        return context;
    }

    void recordDuration(std::chrono::duration<float> duration);
    std::chrono::duration<float> getAvgRunDuration() const;

    virtual void destroyChunk(ChunkBase *chunk) = 0;

protected:
    app::AppContext &context;

    static thread_local std::vector<ChunkBase *> dependencyStack;

private:
    std::atomic<std::chrono::duration<float>> avgRunDuration;
};

}

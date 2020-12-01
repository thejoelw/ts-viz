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

protected:
    app::AppContext &context;

    static std::vector<ChunkBase *> &getDependencyStack();

private:
    std::atomic<std::chrono::duration<float>> avgRunDuration;

    static thread_local std::vector<ChunkBase *> dependencyStack;
};

}

#pragma once

#include "defs/ENABLE_CHUNK_MULTITHREADING.h"
#include "defs/ENABLE_CHUNK_DEBUG.h"

#include <vector>
#if ENABLE_CHUNK_MULTITHREADING
#include <chrono>
#include <atomic>
#endif
#if ENABLE_CHUNK_DEBUG
#include <string>
#include <ostream>
#endif

namespace app { class AppContext; }
namespace series { class ChunkBase; }

namespace series {

class DataSeriesBase {
public:
    struct Registry {
        Registry(app::AppContext &context) {}

        std::vector<DataSeriesBase *> registry;
    };

    DataSeriesBase(app::AppContext &context, bool isTransient);
    virtual ~DataSeriesBase();

    DataSeriesBase(DataSeriesBase const&) = delete;
    DataSeriesBase& operator=(DataSeriesBase const&) = delete;

    app::AppContext &getContext() {
        return context;
    }

#if ENABLE_CHUNK_MULTITHREADING
    void recordDuration(std::chrono::duration<float> duration);
    std::chrono::duration<float> getAvgRunDuration() const;
#endif

#if ENABLE_CHUNK_DEBUG
    struct Meta {
        Meta(const std::string &name, const std::string &trace)
            : name(name)
            , trace(trace)
        {}

        std::string name;
        std::string trace;
    };
    void addMeta(const std::string &name, const std::string &trace);

    virtual void writeDebug(std::ostream &dst) const = 0;
#endif

    virtual void releaseChunk(const ChunkBase *chunk) = 0;

    bool getIsTransient() const {
        return isTransient;
    }

protected:
    app::AppContext &context;

    static std::vector<ChunkBase *> &getDependencyStack();

#if ENABLE_CHUNK_DEBUG
    std::vector<Meta> metas;
#endif

    static thread_local bool dryConstruct;

private:
#if ENABLE_CHUNK_MULTITHREADING
    std::atomic<std::chrono::duration<float>> avgRunDuration;
#endif

    bool isTransient;

    static thread_local std::vector<ChunkBase *> dependencyStack;
};

}

#pragma once

#include <vector>

#include "jw_util/thread.h"

#include "app/appcontext.h"
#include "util/taskscheduler.h"
#include "util/task.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"

namespace series {

template <typename ElementType>
class DataSeries {
public:
    class DynamicWidthException : public jw_util::BaseException {
        friend class DataSeries<ElementType>;

    private:
        DynamicWidthException(const std::string &msg)
            : BaseException(msg)
        {}
    };

    class Chunk {
        friend class DataSeries<ElementType>;

    public:
        static constexpr std::size_t size = (static_cast<std::size_t>(1) << CHUNK_SIZE_LOG2) / sizeof(ElementType);

        Chunk(DataSeries<ElementType> *series, std::size_t index) {
            task.setName(series->getName() + "[" + std::to_string(index) + "]");

            Chunk *prevActiveComputingChunk = activeComputingChunk;
            activeComputingChunk = this;

            auto func = series->getChunkGenerator(index);
            task.setFunction([this, func](util::TaskScheduler &){func(data);});

            assert(activeComputingChunk == this);
            activeComputingChunk = prevActiveComputingChunk;
        }

        bool isDone() const {
            return task.isDone();
        }

        ElementType *getData() {
            unsigned int ref;
            assert(task.isDone(ref));
            return data;
        }

        ElementType *getVolatileData() {
            return data;
        }

        util::Task &getTask() {
            return task;
        }

    private:
        ElementType data[size];
        util::Task task;
    };


    DataSeries(app::AppContext &context)
        : context(context)
    {
        jw_util::Thread::set_main_thread();
    }

    virtual std::string getName() const = 0;

    virtual std::size_t getStaticWidth() const {
        throw DynamicWidthException("Cannot get static width for series");
    }

    Chunk *getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        if (chunks.size() <= chunkIndex) {
            chunks.resize(chunkIndex + 1, 0);
        }
        if (chunks[chunkIndex] == 0) {
            chunks[chunkIndex] = makeChunk(chunkIndex);
        }

        if (activeComputingChunk) {
            activeComputingChunk->task.addDependency(chunks[chunkIndex]->task);
        }

        return chunks[chunkIndex];
    }

    virtual std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) = 0;

protected:
    app::AppContext &context;

    static inline thread_local Chunk *activeComputingChunk;

private:
    std::vector<Chunk *> chunks;

    util::Task *getNearbyTask(std::size_t index) {
        return 0;

        for (std::size_t i = 1; i < 4; i++) {
            if (index >= i) {
                Chunk *neighbor = chunks[index - i];
                if (neighbor) {
                    return &neighbor->task;
                }
            }

            if (index + i < chunks.size()) {
                Chunk *neighbor = chunks[index + i];
                if (neighbor) {
                    return &neighbor->task;
                }
            }
        }

        return 0;
    }

    Chunk *makeChunk(std::size_t index) {
        util::Task *nearbyTask = getNearbyTask(index);
        Chunk *res = context.get<util::Pool<Chunk>>().alloc(this, index);
        if (nearbyTask) {
            res->task.addSimilarTask(*nearbyTask);
        }

        res->task.finishDependency(context.get<util::TaskScheduler>());

        return res;
    }
};

}

#pragma once

#include <vector>

#include "jw_util/thread.h"

#include "app/appcontext.h"
#include "util/taskscheduler.h"
#include "util/task.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"

// TODO: REMOVE
#include "spdlog/spdlog.h"

namespace series {

extern thread_local util::Task *activeTask;

template <typename ElementType>
class DataSeries {
public:
    class Chunk {
        friend class DataSeries<ElementType>;

    public:
        static constexpr std::size_t size = (static_cast<std::size_t>(1) << CHUNK_SIZE_LOG2) / sizeof(ElementType);

        Chunk(DataSeries<ElementType> *series, std::size_t index) {
            jw_util::Thread::assert_main_thread();

            util::Task *prevActiveTask = activeTask;
            activeTask = &task;

            auto func = series->getChunkGenerator(index);

            assert(activeTask == &task);
            activeTask = prevActiveTask;

            task.setFunction([this, func](util::TaskScheduler &){func(data);});
        }

        bool isDone() const {
            return task.isDone();
        }

        ElementType *getData() {
            unsigned int ref;
            if (!task.isDone(ref)) {
                spdlog::info("{} fail", reinterpret_cast<std::uintptr_t>(&task));
            }
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

    Chunk *getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        if (chunks.size() <= chunkIndex) {
            chunks.resize(chunkIndex + 1, 0);
        }
        if (chunks[chunkIndex] == 0) {
            chunks[chunkIndex] = makeChunk(chunkIndex);
        }

        if (activeTask != 0) {
            activeTask->addDependency(chunks[chunkIndex]->task);
        }

        return chunks[chunkIndex];
    }

    virtual std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) = 0;

protected:
    app::AppContext &context;

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

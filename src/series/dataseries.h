#pragma once

#include <vector>

#include "jw_util/thread.h"

#include "app/appcontext.h"
#include "util/taskscheduler.h"
#include "util/task.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"
#define CHUNK_SIZE (static_cast<std::size_t>(1) << CHUNK_SIZE_LOG2)

namespace {

template <typename Type>
struct ChunkAllocator {
    typedef Type value_type;

    ChunkAllocator() {}
    template <typename Other> ChunkAllocator(const ChunkAllocator<Other> &other) {
        (void) other;
    };

    Type *allocate(std::size_t n) {
        return static_cast<Type *>(::operator new(n * sizeof(Type)));
    }
    void deallocate(Type *ptr, std::size_t n) {
        ::operator delete(ptr);
    }
};

template <typename T1, typename T2>
bool operator==(const ChunkAllocator<T1> &a, const ChunkAllocator<T2> &b) {
    (void) a;
    (void) b;
    return true;
}

template <typename T1, typename T2>
bool operator!=(const ChunkAllocator<T1> &a, const ChunkAllocator<T2> &b) {
    return !(a == b);
}

}

namespace series {

extern thread_local util::Task *activeTask;

template <typename ElementType>
class DataSeries {
public:
    class Chunk {
        friend class DataSeries<ElementType>;

    public:
        Chunk(DataSeries<ElementType> *series, std::size_t index) {
            jw_util::Thread::assert_main_thread();

            util::Task *prevActiveTask = activeTask;
            activeTask = &task;

            auto func = series->getChunkGenerator(index);

            assert(activeTask == &task);
            activeTask = prevActiveTask;

            task.setFunction([this, func = std::move(func)](util::TaskScheduler &){func(data);});
        }

        bool isDone() const {
            return task.isDone();
        }

        ElementType *getData() {
            assert(task.isDone());
            return data;
        }

        ElementType *getVolatileData() {
            return data;
        }

        util::Task &getTask() {
            return task;
        }

    private:
        ElementType data[CHUNK_SIZE];
        util::Task task;
    };


    DataSeries(app::AppContext &context)
        : context(context)
    {
        jw_util::Thread::set_main_thread();
    }

    std::shared_ptr<Chunk> getChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        if (chunks.size() <= chunkIndex) {
            chunks.resize(chunkIndex + 1, 0);
        }
        if (!chunks[chunkIndex]) {
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
    std::vector<std::shared_ptr<Chunk>> chunks;

    std::shared_ptr<Chunk> getNearbyChunk(std::size_t index) {
        for (std::size_t i = 1; i < 4; i++) {
            if (index >= i) {
                std::shared_ptr<Chunk> neighbor = chunks[index - i];
                if (neighbor) {
                    return std::move(neighbor);
                }
            }

            if (index + i < chunks.size()) {
                std::shared_ptr<Chunk> neighbor = chunks[index + i];
                if (neighbor) {
                    return std::move(neighbor);
                }
            }
        }

        return 0;
    }

    std::shared_ptr<Chunk> makeChunk(std::size_t index) {
        std::shared_ptr<Chunk> res = std::make_shared<Chunk>(this, index);

        std::shared_ptr<Chunk> nearbyChunk = getNearbyChunk(index);
        if (nearbyChunk) {
            res->task.addSimilarTask(nearbyChunk->task);
        }

        res->task.finishDependency(context.get<util::TaskScheduler>());

        return std::move(res);
    }
};

}

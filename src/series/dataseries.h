#pragma once

#include <vector>

#include "jw_util/thread.h"

#include "series/series.h"
#include "render/seriesrenderer.h"
#include "util/taskscheduler.h"
#include "util/task.h"
#include "util/pool.h"

#include "defs/CHUNK_SIZE_LOG2.h"

namespace series {

template <typename ElementType>
class DataSeries : public Series {
public:
    class DynamicWidthException : public jw_util::BaseException {
        friend class DataSeries<ElementType>;

    private:
        DynamicWidthException(const std::string &msg)
            : BaseException(msg)
        {}
    };

protected:
    class Chunk {
        friend class DataSeries<ElementType>;

    public:
        static constexpr std::size_t size = (static_cast<std::size_t>(1) << CHUNK_SIZE_LOG2) / sizeof(ElementType);

        Chunk(DataSeries<ElementType> *series, std::size_t index) {
            assert(activeComputingChunk == 0);
            activeComputingChunk = this;

            task.setFunction([this, cb = series->getChunkGenerator(index)](util::TaskScheduler &){cb(data);});

            assert(activeComputingChunk == this);
            activeComputingChunk = 0;
        }

        util::Task::Status getStatus() const {
            return task.getStatus();
        }

        ElementType *getData() {
            assert(task.getStatus() == util::Task::Status::Done);
            return data;
        }

    private:
        ElementType data[size];
        util::Task task;
    };

public:

    DataSeries(app::AppContext &context)
        : Series(context)
    {
        jw_util::Thread::set_main_thread();
    }

    ~DataSeries() {
        delete[] renderer;
    }

    void draw(std::size_t begin, std::size_t end, std::size_t stride) override {
        assert(begin <= end);

        if (!renderer) {
            renderer = new render::SeriesRenderer<ElementType>(context);
        }

        static thread_local std::vector<ElementType> sample;
        sample.clear();

        for (std::size_t i = begin; i < end; i += stride) {
            Chunk *chunk = getChunk(i / Chunk::size);
            if (chunk->getStatus() == util::Task::Status::Done) {
                sample.push_back(chunk->getData()[i % Chunk::size]);
            } else {
                sample.push_back(NAN);
            }
        }

        renderer->draw(begin, stride, sample);
    }

    virtual std::size_t getStaticWidth() const {
        throw DynamicWidthException("Cannot get static width for series");
    }

    util::Task *getNearbyTask(std::size_t index) {
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

        res->task.submitTo(context.get<util::TaskScheduler>());

        return res;
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

    ElementType *modifyChunk(std::size_t chunkIndex) {
        jw_util::Thread::assert_main_thread();

        if (chunks.size() <= chunkIndex) {
            chunks.resize(chunkIndex + 1, 0);
        }
        if (chunks[chunkIndex] == 0) {
            chunks[chunkIndex] = makeChunk(chunkIndex);
        }

        assert(!activeComputingChunk);

        chunks[chunkIndex]->task.rerun(context.get<util::TaskScheduler>());

        return chunks[chunkIndex]->data;
    }

    virtual std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) = 0;

private:
    std::vector<Chunk *> chunks;

    static inline thread_local Chunk *activeComputingChunk;

    render::SeriesRenderer<ElementType> *renderer = 0;
};

}

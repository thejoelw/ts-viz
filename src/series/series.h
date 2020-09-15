#pragma once

#include "taskflow/taskflow/taskflow.hpp"

#include "app/appcontext.h"
#include "util/refset.h"
#include "util/taskscheduler.h"

namespace series {

class Series {
public:
    class Registry {
    public:
        Registry(app::AppContext &context) {
            (void) context;
        }

        std::vector<util::RefSet<Series>> &getLayers() {
            return layers;
        }

        util::RefSet<Series> &getDepthSet(std::size_t depth) {
            if (layers.size() <= depth) {
                layers.resize(depth + 1);
            }
            return layers[depth];
        }

    private:
        std::vector<util::RefSet<Series>> layers;
    };

    Series(app::AppContext &context)
        : context(context)
        , task(context.get<tf::Taskflow>().emplace([this]() {
            if (this->requestedBegin < this->requestedEnd) {
                this->compute(this->requestedBegin, this->requestedEnd);
            }
        }))
    {
        context.get<Registry>().getDepthSet(depth).add(this);
    }

    ~Series() {
        context.get<Registry>().getDepthSet(depth).remove(this);
    }

    virtual void propogateRequest() = 0;
    virtual void compute(std::size_t begin, std::size_t end) = 0;
    virtual void draw(std::size_t begin, std::size_t end, std::size_t stride) = 0;

    void dependsOn(const Series *input) {
        task.succeed(input->task);

        if (depth <= input->depth) {
            context.get<Registry>().getDepthSet(depth).remove(this);
            depth = input->depth + 1;
            context.get<Registry>().getDepthSet(depth).add(this);
        }
    }

    void request(std::size_t begin, std::size_t end) {
        if (begin >= end) {
            return;
        }

        if (requestedBegin == requestedEnd) {
            requestedBegin = begin;
            requestedEnd = end;
        } else {
            if (begin < requestedBegin) {
                requestedBegin = begin;
            }
            if (end > requestedEnd) {
                requestedEnd = end;
            }
        }
    }

    std::size_t getComputedBegin() const {
        return computedBegin;
    }
    std::size_t getComputedEnd() const {
        return computedEnd;
    }

    std::size_t getDepth() const {
        return depth;
    }

protected:
    app::AppContext &context;

    std::size_t computedBegin = 0;
    std::size_t computedEnd = 0;

    std::size_t requestedBegin = 0;
    std::size_t requestedEnd = 0;

    std::size_t depth = 0;

private:
    tf::Task task;
};

}

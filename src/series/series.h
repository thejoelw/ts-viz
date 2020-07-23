#pragma once

#include <vector>

#include "taskflow/taskflow/taskflow.hpp"
#include "PicoSHA2/picosha2.h"

#include "graphics/glbuffer.h"

namespace series {

template <typename ElementType>
class Series {
public:
    Series(tf::Task task)
        : task(task)
    {}

    virtual void request(std::size_t begin, std::size_t end) = 0;
    virtual void compute() = 0;

    ElementType *getData(std::size_t begin) {
        assert(begin >= dataOffset);
        return data.data() + (begin - dataOffset);
    }

protected:
    std::vector<ElementType> data;
    std::size_t dataOffset = 0;

    std::size_t requestedBegin = 0;
    std::size_t requestedEnd = 0;

private:
    tf::Task task;

//    graphics::GlBuffer<ElementType> remoteBuffer;
};

}

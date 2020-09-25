#pragma once

#include "taskflow/taskflow/taskflow.hpp"

#include "app/appcontext.h"
#include "util/refset.h"
#include "util/taskscheduler.h"

namespace series {

class Series {
public:
    Series(app::AppContext &context)
        : context(context)
    {}

    ~Series() {}

    virtual void draw(std::size_t begin, std::size_t end, std::size_t stride) = 0;

protected:
    app::AppContext &context;
};

}

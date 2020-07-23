#ifndef JWUTIL_WORKQUEUE_H
#define JWUTIL_WORKQUEUE_H

#include "workqueuebase.h"

namespace jw_util
{

template <typename... ArgTypes>
class WorkQueue : public WorkQueueBase<WorkQueue<ArgTypes...>, ArgTypes...>
{
    typedef WorkQueueBase<WorkQueue<ArgTypes...>, ArgTypes...> BaseType;
    friend BaseType;

public:
    using BaseType::BaseType;

private:
    void wait(std::unique_lock<std::mutex> &lock)
    {
        BaseType::conditional_variable.wait(lock);
    }
};

}

#endif // JWUTIL_WORKQUEUE_H

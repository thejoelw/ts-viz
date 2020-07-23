#ifndef JWUTIL_WORKQUEUEINSOMNIAC_H
#define JWUTIL_WORKQUEUEINSOMNIAC_H

#include <chrono>

#include "workqueuebase.h"

namespace jw_util
{

template <typename... ArgTypes>
class WorkQueueInsomniac : public WorkQueueBase<WorkQueueInsomniac<ArgTypes...>, ArgTypes...>
{
    typedef WorkQueueBase<WorkQueueInsomniac<ArgTypes...>, ArgTypes...> BaseType;
    friend BaseType;

public:
    using WorkQueueBase<WorkQueueInsomniac<ArgTypes...>, ArgTypes...>::WorkQueueBase;

    void set_wakeup_worker(jw_util::MethodCallback<> worker)
    {
        wakeup_worker = worker;
    }

    template <typename DurationRep, typename DurationPeriod>
    void set_wakeup_interval(std::chrono::duration<DurationRep, DurationPeriod> duration)
    {
        next_wakeup = std::chrono::steady_clock::now();
        wakeup_interval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration);
    }

private:
    jw_util::MethodCallback<> wakeup_worker;

    std::chrono::steady_clock::time_point next_wakeup = std::chrono::steady_clock::time_point::max();
    std::chrono::steady_clock::duration wakeup_interval = std::chrono::steady_clock::duration::max();

    void wait(std::unique_lock<std::mutex> &lock)
    {
        std::cv_status status = BaseType::conditional_variable.wait_until(lock, next_wakeup);

        if (status == std::cv_status::timeout)
        {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            next_wakeup += wakeup_interval;
            if (next_wakeup < now) {next_wakeup = now;}
            wakeup_worker.call();
        }
    }
};

}

#endif // JWUTIL_WORKQUEUEINSOMNIAC_H

#pragma once

#include <cmath>
#include <functional>
#include <vector>
#include <chrono>

#include "jw_util/baseexception.h"

namespace util {

class TaskScheduler;

class Task {
    friend class TaskScheduler;

public:
    enum class Status {
        Pending,
        Queued,
        Running,
        Done,
    };

    Status getStatus() const {
        return status;
    }

    void addSimilarTask(Task &similar);
    void addDependency(Task &dep);
    void setFunction(const std::function<void(TaskScheduler &)> &newFunc);

    void submitTo(TaskScheduler &scheduler);
    void rerun(TaskScheduler &scheduler);

private:
    std::atomic<Status> status = Status::Pending;

    std::function<void(TaskScheduler &)> func;

    double orderingSum = 0.0;
    std::size_t orderingCount = 0;

    double selfDuration = 0.0;
    double followingDuration = -1.0;

    std::atomic<std::size_t> waitingCount = 1;
    std::vector<Task *> dependents;

    void call(TaskScheduler &scheduler);

    double getOrdering() const;

    double getCriticalPathDuration();
};

}

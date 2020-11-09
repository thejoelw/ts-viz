#pragma once

#include <cmath>
#include <functional>
#include <vector>
#include <chrono>

#include "jw_util/baseexception.h"

#include "util/spinlock.h"

namespace util {

class TaskScheduler;

class Task {
    friend class TaskScheduler;

public:
    ~Task() {
        std::lock_guard<SpinLock> lock(dependentsMutex);
//        assert(dependents.empty());
        assert(isDone());
    }

    void addSimilarTask(Task &similar);
    void setFunction(const std::function<void(TaskScheduler &)> &newFunc);
    void addDependency(Task &dep);
    void addDependency();
    void finishDependency(TaskScheduler &scheduler);

    bool isDone() const {
        return depCounter == static_cast<unsigned int>(-1);
    }

private:
    std::function<void(TaskScheduler &)> func;

    double orderingSum = 0.0;
    std::size_t orderingCount = 0;

    double selfDuration = 0.0;
    double followingDuration = -1.0;

    std::atomic<unsigned int> depCounter = 1;
    SpinLock dependentsMutex;
    std::vector<Task *> dependents;

    void call(TaskScheduler &scheduler);

    double getOrdering() const;
    double getCriticalPathDuration();
};

}

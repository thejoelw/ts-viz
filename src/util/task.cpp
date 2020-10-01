#include "task.h"

#include "util/taskscheduler.h"

#if TASKSCHEDULER_ENABLE_DEBUG_OUTPUT
#include "spdlog/spdlog.h"
#endif

namespace util {

void Task::addSimilarTask(Task &similar) {
    orderingSum += similar.getCriticalPathDuration();
    orderingCount++;
}

void Task::setFunction(const std::function<void(TaskScheduler &)> &newFunc) {
    func = newFunc;
}

void Task::addDependency(Task &dep) {
    // Says dep must run before this

    depCounter++;
    dep.dependents.push_back(this);
}

void Task::addDependency() {
    depCounter++;
}

void Task::finishDependency(TaskScheduler &scheduler) {
    unsigned int prevValue = depCounter--;
    assert(prevValue != 0);
    if (prevValue == 1) {
        scheduler.addTask(this);
    }
}

/*
void Task::rerun(TaskScheduler &scheduler) {
    unsigned int prevValue = depCounter.fetch_and(~doneMask);
    if (prevValue & doneMask) {
        for (Task *dep : dependents) {
            dep->rerunAfter(scheduler);
        }
    }
    if (prevValue == doneMask) {
        scheduler.addTask(this);
    }
}

void Task::rerunAfter(TaskScheduler &scheduler) {
    unsigned int prevValue = depCounter.fetch_and(~doneMask);
    if (prevValue & doneMask) {
        depCounter++;
        for (Task *dep : dependents) {
            dep->rerunAfter(scheduler);
        }
    }
}
*/

void Task::call(TaskScheduler &scheduler) {
    auto t1 = std::chrono::high_resolution_clock::now();
    func(scheduler);
    auto t2 = std::chrono::high_resolution_clock::now();

    double durationSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    selfDuration = durationSeconds;

    for (Task *dep : dependents) {
        dep->finishDependency(scheduler);
    }
}

double Task::getOrdering() const {
    return orderingSum / orderingCount;
}

double Task::getCriticalPathDuration() {
    if (followingDuration == -1.0) {
        followingDuration = 0.0;
        for (Task *dep : dependents) {
            double depDur = dep->getCriticalPathDuration();
            if (depDur > followingDuration) {
                followingDuration = depDur;
            }
        }
    }
    return selfDuration + followingDuration;
}

}

#include "task.h"

#include "util/taskscheduler.h"

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

    std::lock_guard<SpinLock> lock(dep.dependentsMutex);

    dep.dependents.push_back(this);

    if (!dep.isDone()) {
        addDependency();
    }
}

void Task::addDependency() {
    unsigned int prevValue = depCounter++;
    assert(prevValue > 0);
}

void Task::finishDependency(TaskScheduler &scheduler) {
    unsigned int prevValue = depCounter--;
    assert(prevValue != 0);
    assert(prevValue != static_cast<unsigned int>(-1));
    if (prevValue == 1) {
        scheduler.addTask(this);
    }
}

void Task::call(TaskScheduler &scheduler) {
    assert(depCounter == 0);

    auto t1 = std::chrono::high_resolution_clock::now();
    func(scheduler);
    auto t2 = std::chrono::high_resolution_clock::now();

    double durationSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    selfDuration = durationSeconds;

    std::lock_guard<SpinLock> lock(dependentsMutex);

    unsigned int prevValue = depCounter--;
    assert(prevValue == 0);

    for (Task *dep : dependents) {
        dep->finishDependency(scheduler);
    }
}

double Task::getOrdering() const {
    if (orderingCount > 0) {
        return orderingSum / orderingCount;
    } else {
        return 0.0;
    }
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

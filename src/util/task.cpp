#include "task.h"

#include "util/taskscheduler.h"

#include "spdlog/spdlog.h"

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

    spdlog::info("{} {} addDep {} pre-lock", reinterpret_cast<std::uintptr_t>(this), depCounter, reinterpret_cast<std::uintptr_t>(&dep));

    std::lock_guard<SpinLock> lock(dep.dependentsMutex);

    spdlog::info("{} {} addDep {} post-lock", reinterpret_cast<std::uintptr_t>(this), depCounter, reinterpret_cast<std::uintptr_t>(&dep));

    dep.dependents.push_back(this);

    if (!dep.isDone()) {
        addDependency();
    }
}

void Task::addDependency() {
    spdlog::info("{} {} addDep pre-inc", reinterpret_cast<std::uintptr_t>(this), depCounter);
    unsigned int prevValue = depCounter++;
    spdlog::info("{} {} addDep post-inc", reinterpret_cast<std::uintptr_t>(this), depCounter);
    assert(prevValue > 0);
}

void Task::finishDependency(TaskScheduler &scheduler) {
    spdlog::info("{} {} finDep pre-dec", reinterpret_cast<std::uintptr_t>(this), depCounter);
    unsigned int prevValue = depCounter--;
    spdlog::info("{} {} finDep post-dec", reinterpret_cast<std::uintptr_t>(this), depCounter);
    assert(prevValue != 0);
    assert(prevValue != static_cast<unsigned int>(-1));
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
    assert(depCounter == 0);

    auto t1 = std::chrono::high_resolution_clock::now();
    func(scheduler);
    auto t2 = std::chrono::high_resolution_clock::now();

    double durationSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    selfDuration = durationSeconds;

    spdlog::info("{} {} call pre-lock", reinterpret_cast<std::uintptr_t>(this), depCounter);
    std::lock_guard<SpinLock> lock(dependentsMutex);
    spdlog::info("{} {} call post-lock", reinterpret_cast<std::uintptr_t>(this), depCounter);

    unsigned int prevValue = depCounter--;
    spdlog::info("{} {} call post-dec", reinterpret_cast<std::uintptr_t>(this), depCounter);
    assert(prevValue == 0);

    for (Task *dep : dependents) {
        dep->finishDependency(scheduler);
    }

    spdlog::info("{} {} call post-loop", reinterpret_cast<std::uintptr_t>(this), depCounter);
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

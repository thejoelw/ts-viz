#include "task.h"

#include "util/taskscheduler.h"

namespace util {

void Task::addSimilarTask(Task &similar) {
    orderingSum += similar.getCriticalPathDuration();
    orderingCount++;
}

void Task::addDependency(Task &dep) {
    // Says dep must run before this

    waitingCount++;
    dep.dependents.push_back(this);
}

void Task::setFunction(const std::function<void(TaskScheduler &)> &newFunc) {
    func = newFunc;
}

void Task::submitTo(TaskScheduler &scheduler) {
    assert(status == Task::Status::Pending);

    if (!--waitingCount) {
        status = Task::Status::Queued;

        scheduler.addTask(this);
    }
}

void Task::rerun(TaskScheduler &scheduler) {
    if (status == Status::Done) {
        status = Status::Queued;
        scheduler.addTask(this);

        for (Task *dep : dependents) {
            dep->rerun(scheduler);
        }
    }
}

void Task::call(TaskScheduler &scheduler) {
    assert(status == Status::Queued);
    status = Status::Running;

    auto t1 = std::chrono::high_resolution_clock::now();
    func(scheduler);
    auto t2 = std::chrono::high_resolution_clock::now();

    double durationSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    selfDuration = durationSeconds;

    assert(status == Status::Running);
    status = Status::Done;

    for (Task *dep : dependents) {
        dep->submitTo(scheduler);
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

#include "taskscheduler.h"

#include "util/task.h"

namespace util {

TaskScheduler::TaskScheduler(app::AppContext &context, std::size_t numThreads)
    : numThreads(numThreads)
    , threads(numThreads ? new std::thread[numThreads] : 0)
{
    (void) context;

    for (std::size_t i = 0; i < numThreads; i++) {
        threads[i] = std::thread(&TaskScheduler::worker, this);
    }
}

TaskScheduler::~TaskScheduler() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        (void) lock;
        running = false;
    }

    cond.notify_all();

    for (unsigned int i = 0; i < numThreads; i++) {
        threads[i].join();
    }

    delete[] threads;
}

void TaskScheduler::addTask(Task *task) {
    assert(task->waitingCount == 0);

    std::unique_lock<std::mutex> lock(mutex);
    queue.push(task);
    cond.notify_one();
}

bool TaskScheduler::TaskOrder::operator()(const Task *left, const Task *right) const {
    return left->getOrdering() < right->getOrdering();
}

void TaskScheduler::workerFunc(TaskScheduler *scheduler) {
    scheduler->worker();
}

void TaskScheduler::worker() {
    std::unique_lock<std::mutex> lock(mutex);
    while (true) {
        if (queue.empty()) {
            if (!running) {break;}
            cond.wait(lock);
        } else {
            Task *task = queue.top();
            queue.pop();

            lock.unlock();
            task->call(*this);
            lock.lock();
        }
    }
}

}

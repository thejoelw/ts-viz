#pragma once

#include <functional>
#include <unordered_map>
#include <thread>
#include <array>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace util {

class Scheduler;

class Task {
    friend class Scheduler;

public:
    Task(std::function<void(Scheduler &)> func)
        : func(func)
    {}

private:
    std::function<void(Scheduler &)> func;
    std::condition_variable finishCond;

    enum class Status {
        Pending,
        Queued,
        Done,
    };
    Status status = Status::Pending;

    double avgCriticalPathDuration = 0.0;

    double lastDuration;
    double lastCriticalPathDuration;
    std::vector<Task *> lastDependents;

    double getOrdering() const {
        return avgCriticalPathDuration;
    }

    void addDependency(Task &dep) {
        // dep must run before this

        dep.lastDependents.push_back(this);

        if (dep.getOrdering() <= this->getOrdering()) {
            dep.avgCriticalPathDuration = nextafter(avgCriticalPathDuration, INFINITY);
        }
    }

    void recordDuration(double duration) {
        assert(status == Status::Done);
        lastDuration = duration;
        lastCriticalPathDuration = -1.0;
    }

    double getCriticalPathDuration() {
        if (lastCriticalPathDuration == -1.0) {
            lastCriticalPathDuration = 0.0;
            for (Task *dep : lastDependents) {
                double depDur = dep->getCriticalPathDuration();
                if (depDur > lastCriticalPathDuration) {
                    lastCriticalPathDuration = depDur;
                }
            }
            lastCriticalPathDuration += lastDuration;
        }
        return lastCriticalPathDuration;
    }

    void reset() {
        assert(status == Status::Done);
        status = Status::Pending;
        avgCriticalPathDuration = avgCriticalPathDuration * 0.9 + getCriticalPathDuration() * 0.1;
        lastDependents.clear();
    }
};

class Scheduler {
public:
    Scheduler(std::size_t numThreads = std::thread::hardware_concurrency())
        : numThreads(numThreads)
        , threads(numThreads ? new std::thread[numThreads] : 0)
    {
        for (std::size_t i = 0; i < numThreads; i++) {
            threads[i] = std::thread(&Scheduler::worker, this);
        }
    }

    ~Scheduler() {
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

    template <typename... TaskTypes>
    void run(TaskTypes &...tasks) {
        Task *thisTask = currentTask;

        std::array<Task *, sizeof...(TaskTypes)> arr = {{ &tasks... }};

        std::size_t pushed = 0;

        std::unique_lock<std::mutex> lock(mutex);
        for (Task *task : arr) {
            if (task->status == Task::Status::Pending) {
                thisTask->addDependency(*task);
                queue.push(task);
                task->status = Task::Status::Queued;
                pushed++;
            }
        }

        if (pushed < numThreads) {
            // Start at 1 because we're going to process an entry below
            for (std::size_t i = 1; i < pushed; i++) {
                cond.notify_one();
            }
        } else {
            cond.notify_all();
        }

        while (!queue.empty() && queue.top()->getOrdering() > thisTask->getOrdering()) {
            processQueueEntry(lock);
        }

        for (Task *task : arr) {
            while (task->status != Task::Status::Done) {
                task->finishCond.wait(lock);
            }
        }

        currentTask = thisTask;
    }

    void reset() {
        for (Task *task : completedTasks) {
            task->reset();
        }
        completedTasks.clear();
    }

private:
    static thread_local Task *currentTask;

    std::size_t numThreads;
    std::thread *threads;
    bool running = true;

    std::mutex mutex;
    std::condition_variable cond;

    struct TaskOrder {
        bool operator()(const Task *left, const Task *right) const {
            return left->getOrdering() < right->getOrdering();
        }
    };
    std::priority_queue<Task *, std::vector<Task *>, TaskOrder> queue;

    std::vector<Task *> completedTasks;

    static void workerFunc(Scheduler *scheduler) {
        scheduler->worker();
    }

    void worker() {
        Task workerTask([](Scheduler &){});
        workerTask.avgCriticalPathDuration = -1.0;
        currentTask = &workerTask;

        std::unique_lock<std::mutex> lock(mutex);
        while (true) {
            if (queue.empty()) {
                if (!running) {break;}
                cond.wait(lock);
            } else {
                processQueueEntry(lock);
            }
        }
    }

    void processQueueEntry(std::unique_lock<std::mutex> &lock) {
        assert(!queue.empty());
        Task *task = queue.top();
        queue.pop();
        assert(task->status == Task::Status::Queued);
        lock.unlock();

        auto t1 = std::chrono::high_resolution_clock::now();

        currentTask = task;
        task->func(*this);
        assert(currentTask == task);

        auto t2 = std::chrono::high_resolution_clock::now();
        double durationSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

        lock.lock();
        assert(task->status == Task::Status::Queued);
        task->status = Task::Status::Done;
        task->finishCond.notify_all();
        task->recordDuration(durationSeconds);
        completedTasks.push_back(task);
    }
};

}

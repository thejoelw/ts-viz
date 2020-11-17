#pragma once

#include <cstdlib>
#include <thread>
#include <queue>

namespace app { class AppContext; }

namespace util {

template <typename TaskType>
class TaskScheduler {
public:
    TaskScheduler(app::AppContext &context, std::size_t numThreads = std::thread::hardware_concurrency())
        : numThreads(numThreads)
        , threads(numThreads ? new std::thread[numThreads] : 0)
    {
        (void) context;

        for (std::size_t i = 0; i < numThreads; i++) {
            threads[i] = std::thread(&TaskScheduler::worker, this);
        }
    }

    ~TaskScheduler() {
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

    void addTask(TaskType *task) {
        if (numThreads) {
            std::unique_lock<std::mutex> lock(mutex);

            queue.push(task);
            cond.notify_one();
        } else {
            task->exec();
        }
    }


private:
    std::size_t numThreads;
    std::thread *threads;
    bool running = true;

    struct TaskOrder {
        bool operator()(const TaskType *left, const TaskType *right) const {
            return left->getOrdering() < right->getOrdering();
        }
    };
    std::priority_queue<TaskType *, std::vector<TaskType *>, TaskOrder> queue;

    std::mutex mutex;
    std::condition_variable cond;

    static void workerFunc(TaskScheduler *scheduler) {
        scheduler->worker();
    }

    void worker() {
        std::unique_lock<std::mutex> lock(mutex);
        while (true) {
            if (queue.empty()) {
                if (!running) {break;}
                cond.wait(lock);
            } else {
                TaskType *task = queue.top();
                queue.pop();

                lock.unlock();
                task->exec();
                lock.lock();
            }
        }
    }
};

}

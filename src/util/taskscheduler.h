#pragma once

#include <cstdlib>
#include <thread>
#include <queue>

namespace app { class AppContext; }

namespace util {

class Task;

class TaskScheduler {
public:
    TaskScheduler(app::AppContext &context, std::size_t numThreads = std::thread::hardware_concurrency());
    ~TaskScheduler();

    void addTask(Task *task);

private:
    static thread_local Task *currentTask;

    std::size_t numThreads;
    std::thread *threads;
    bool running = true;

    struct TaskOrder {
        bool operator()(const Task *left, const Task *right) const;
    };
    std::priority_queue<Task *, std::vector<Task *>, TaskOrder> queue;

    std::mutex mutex;
    std::condition_variable cond;

    static void workerFunc(TaskScheduler *scheduler);

    void worker();
};

}

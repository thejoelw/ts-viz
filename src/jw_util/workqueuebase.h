#ifndef JWUTIL_WORKQUEUEBASE_H
#define JWUTIL_WORKQUEUEBASE_H

#include <array>
#include <tuple>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace jw_util
{

template <typename Derived, typename... ArgTypes>
class WorkQueueBase
{
public:
    struct construct_paused_t {};
    static constexpr construct_paused_t construct_paused {};

    WorkQueueBase(unsigned int num_threads)
        : WorkQueueBase(num_threads, construct_paused)
    {
        start();
    }

    WorkQueueBase(unsigned int num_threads, construct_paused_t)
        : num_threads(num_threads)
        , threads(num_threads ? new std::thread[num_threads] : 0)
        , running(false)
    {}

    ~WorkQueueBase()
    {
        if (running)
        {
            pause();
        }

        delete[] threads;
    }

    void push(void (*method)(ArgTypes...), ArgTypes... args)
    {
        assert(running);

        if (num_threads)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                (void) lock;
                queue.emplace(method, std::forward<ArgTypes>(args)...);
            }
            conditional_variable.notify_one();
        }
        else
        {
            (*method)(std::forward<ArgTypes>(args)...);
        }
    }

    void start()
    {
        assert(!running);

        running = true;

        for (unsigned int i = 0; i < num_threads; i++)
        {
            threads[i] = std::thread(&WorkQueueBase<Derived, ArgTypes...>::loop, this);
        }
    }

    void pause()
    {
        assert(running);

        {
            std::lock_guard<std::mutex> lock(mutex);
            (void) lock;
            running = false;
        }

        conditional_variable.notify_all();

        for (unsigned int i = 0; i < num_threads; i++)
        {
            threads[i].join();
        }
    }

    std::size_t getQueueSize() const {
        return queue.size();
    }

protected:
    typedef std::tuple<void (*)(ArgTypes...), ArgTypes...> TupleType;

    std::size_t num_threads;
    std::thread *threads;

    std::mutex mutex;
    std::condition_variable conditional_variable;

    std::queue<TupleType> queue;

    bool running;

    void loop()
    {
        assert(num_threads);

        std::unique_lock<std::mutex> lock(mutex);
        while (true)
        {
            if (queue.empty())
            {
                if (!running) {break;}
                get_derived()->wait(lock);
            }
            else
            {
                TupleType args = std::move(queue.front());
                queue.pop();

                lock.unlock();
                std::apply(dispatch, std::move(args));
                lock.lock();
            }
        }
    }

    static void dispatch(void (*method)(ArgTypes...), ArgTypes... args)
    {
        (*method)(std::forward<ArgTypes>(args)...);
    }

    Derived *get_derived()
    {
        return static_cast<Derived *>(this);
    }
};

}

#endif // JWUTIL_WORKQUEUEBASE_H

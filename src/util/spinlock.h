#pragma once

#include <atomic>
//#include <thread>

namespace util {

class SpinLock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
//            std::this_thread::yield();
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

}

#ifndef JWUTIL_THREAD_H
#define JWUTIL_THREAD_H

#include <thread>
#include <cassert>

namespace jw_util
{

class Thread
{
public:
    Thread() = delete;

    static void set_main_thread()
    {
#ifndef NDEBUG
        if (set_main_thread_id)
        {
            assert(main_thread_id == std::this_thread::get_id());
        }
        else
        {
            set_main_thread_id = true;
        }

        main_thread_id = std::this_thread::get_id();
#endif
    }

    static void assert_main_thread()
    {
#ifndef NDEBUG
        assert(set_main_thread_id);
        assert(std::this_thread::get_id() == main_thread_id);
#endif
    }
    static void assert_child_thread()
    {
#ifndef NDEBUG
        assert(set_main_thread_id);
        assert(std::this_thread::get_id() != main_thread_id);
#endif
    }

private:
#ifndef NDEBUG
    static std::thread::id main_thread_id;
    static bool set_main_thread_id;
#endif
};

}

#endif // JWUTIL_THREAD_H

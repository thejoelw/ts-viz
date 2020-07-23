#include "thread.h"

namespace jw_util
{

#ifndef NDEBUG
std::thread::id Thread::main_thread_id;
bool Thread::set_main_thread_id = false;
#endif

}

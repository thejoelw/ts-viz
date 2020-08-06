#include "taskscheduler.h"

namespace util {

thread_local Task *Scheduler::currentTask = 0;

}

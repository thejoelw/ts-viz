#include "appcontext.h"

#include "log.h"

namespace app {

void AppContext::log(LogLevel level, const std::string &msg) {
    switch (level) {
        case LogLevel::Trace: SPDLOG_TRACE(msg); break;
        case LogLevel::Info: SPDLOG_INFO(msg); break;
        case LogLevel::Warning: SPDLOG_WARN(msg); break;
        case LogLevel::Error: SPDLOG_ERROR(msg); break;
    }
}

}

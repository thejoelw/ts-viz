#pragma once

#include <cstdlib>

#include "log.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "app/appcontext.h"

namespace stream {

template <typename ReceiverClass>
class JsonUnwrapper {
public:
    JsonUnwrapper(app::AppContext &context)
        : context(context)
    {}

    void recvLine(const char *data, std::size_t size) {
        rapidjson::Document row;

        if (row.Parse(data, size).HasParseError()) {
            SPDLOG_WARN("Discarding json record {} because of parsing error: {}", std::string(data, size), rapidjson::GetParseError_En(row.GetParseError()));
        } else {
            context.get<ReceiverClass>().recvRecord(row);
        }
    }

    void yield() {
        context.get<ReceiverClass>().yield();
    }

    void end() {
        context.get<ReceiverClass>().end();
    }

private:
    app::AppContext &context;
};

}

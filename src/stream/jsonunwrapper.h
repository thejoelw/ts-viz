#pragma once

#include <cstdlib>

#include "spdlog/logger.h"
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/error/en.h"

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
            context.get<spdlog::logger>().warn("Parser error: {}", rapidjson::GetParseError_En(row.GetParseError()));
        } else {
//            context.get<spdlog::logger>().info("Received record: {}", std::string(data, size));
            context.get<ReceiverClass>().recvRecord(row);
        }
    }

    void end() {
        context.get<ReceiverClass>().end();
    }

private:
    app::AppContext &context;
};

}

#pragma once

#include <cstdlib>

#include "spdlog/spdlog.h"
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
            spdlog::warn("Parser error: {}", rapidjson::GetParseError_En(row.GetParseError()));
            spdlog::warn("For line: {}", std::string(data, size));
        } else {
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

#pragma once

#include <cstdlib>

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/error/en.h"

#include "app/appcontext.h"

namespace stream {

template <typename ReceiverClass>
class JsonUnwrapper {
public:
    JsonUnwrapper(app::AppContext &context)
        : receiver(context.get<ReceiverClass>())
    {}

    void recvLine(const char *data, std::size_t size) {
        rapidjson::Document row;

        if (row.Parse(data, size).HasParseError()) {
            fprintf(stderr, "Parser error: %s\n", rapidjson::GetParseError_En(row.GetParseError()));
            return;
        }

        receiver.recvRecord(row);
    }

private:
    ReceiverClass &receiver;
};

}

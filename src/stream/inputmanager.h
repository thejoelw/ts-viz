#pragma once

#include <unordered_map>

#include "rapidjson/document.h"

#include "series/type/inputseries.h"

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"

namespace stream {

class InputManager {
public:
    InputManager(app::AppContext &context);

    void recvRecord(const rapidjson::Document &row);
    void end();

private:
    app::AppContext &context;

    std::size_t index = 0;

    std::unordered_map<std::string, series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *> inputs;
};

}

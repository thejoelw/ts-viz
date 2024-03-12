#pragma once

#include <unordered_map>

#include "series/type/inputseries.h"
#include "stream/drawing.h"

namespace stream {

class DrawingManager {
public:
    DrawingManager(app::AppContext &context);

    void resetDrawing(char code);
    void addPoint(glm::vec2 pt);
    Drawing &getStream(const std::string &name);

private:
    app::AppContext &context;

    std::unordered_map<std::string, Drawing> streams;
    Drawing *curStream = nullptr;
};

}

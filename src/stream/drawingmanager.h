#pragma once

#include <unordered_map>

#include "graphics/glm.h"
#include "glm/vec2.hpp"

#include "app/appcontext.h"
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

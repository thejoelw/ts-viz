#pragma once

#include <unordered_map>

#include "graphics/glm.h"
#include "glm/vec2.hpp"

#include "app/appcontext.h"
#include "stream/drawing.h"

namespace stream {

class DrawingManager {
public:
    DrawingManager(app::AppContext &context)
        : context(context)
    {}

    void resetDrawing(char code) {
        curStream = &getStream(std::string(1, code));
        curStream->reset();
    }

    void addPoint(glm::vec2 pt) {
        if (curStream != nullptr) {
            curStream->addPoint(pt);
        }
    }

    Drawing &getStream(const std::string &name) {
        return streams[name];
    }

private:
    app::AppContext &context;

    std::unordered_map<std::string, Drawing> streams;
    Drawing *curStream = nullptr;
};

}

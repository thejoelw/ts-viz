#include "drawingmanager.h"

namespace stream {

DrawingManager::DrawingManager(app::AppContext &context)
    : context(context)
{}

void DrawingManager::resetDrawing(char code) {
    curStream = &getStream(std::string(1, code));
    curStream->reset();
}

void DrawingManager::addPoint(glm::vec2 pt) {
    if (curStream != nullptr) {
        curStream->addPoint(pt);
    }
}

Drawing &DrawingManager::getStream(const std::string &name) {
    return streams[name];
}

}

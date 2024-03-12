#include "drawingmanager.h"

#include "app/appcontext.h"
#include "program/resolver.h"
#include "log.h"
#include "util/jsontostring.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "render/camera.h"
#endif

namespace stream {

DrawingManager::DrawingManager(app::AppContext &context)
    : context(context)
{}

void DrawingManager::resetDrawing(char code) {
    curStream = &getStream(std::string(code, 1));
    curStream->reset();
}

void DrawingManager::addPoint(glm::vec2 pt) {
    if (curStream != nullptr) {
        curStream->addPoint(pt);
    }
}

DrawingStream &DrawingManager::getStream(const std::string &name) {
    return streams[name];
}

}

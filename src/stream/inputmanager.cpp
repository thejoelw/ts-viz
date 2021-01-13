#include "inputmanager.h"

#include "app/appcontext.h"
#include "program/resolver.h"
#include "log.h"
#include "util/jsontostring.h"

#include "defs/ENABLE_GRAPHICS.h"
#if ENABLE_GRAPHICS
#include "render/camera.h"
#endif

namespace stream {

InputManager::InputManager(app::AppContext &context)
    : context(context)
{}

void InputManager::recvRecord(const rapidjson::Document &row) {
    if (!row.IsObject()) {
        SPDLOG_WARN("Discarding input line {} because it should be an object, but instead was {}", index, util::jsonToStr(row));
        return;
    }

    for (rapidjson::Value::ConstMemberIterator it = row.MemberBegin(); it != row.MemberEnd(); ++it) {
        std::string key(it->name.GetString(), it->name.GetStringLength());
        double value;
        switch (it->value.GetType()) {
            case rapidjson::kNullType: value = NAN; break;
            case rapidjson::kNumberType: value = it->value.GetDouble(); break;
            default:
                SPDLOG_WARN("Received input record entry with key {} with invalid value type {}", key, static_cast<unsigned int>(it->value.GetType()));
                continue;
        }

        series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *&in = inputs[key];
        if (!in) {
            std::vector<program::ProgObj> args {
                program::ProgObj(key)
            };
            program::ProgObj po = context.get<program::Resolver>().call("input", args);
            series::DataSeries<INPUT_SERIES_ELEMENT_TYPE> *ds = std::get<series::DataSeries<INPUT_SERIES_ELEMENT_TYPE> *>(po);
            in = dynamic_cast<series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *>(ds);

            SPDLOG_INFO("Received new input record entry key {}", key);
        }

        in->set(index, static_cast<INPUT_SERIES_ELEMENT_TYPE>(value));
    }

#if ENABLE_GRAPHICS
    if (context.has<render::Camera>()) {
        render::Camera &cam = context.get<render::Camera>();
        if (index <= cam.getMax().x && (index + 1) > cam.getMax().x) {
            cam.getMax().x = index + 1;
        }
    }
#endif

    index++;
}

void InputManager::end() {
    assert(running);
    running = false;

    for (const std::pair<std::string, series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *> entry : inputs) {
        entry.second->propagateUntil(index);
    }
}


}

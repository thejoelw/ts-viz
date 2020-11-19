#include "inputmanager.h"

#include "program/resolver.h"
#include "spdlog/logger.h"
#include "render/camera.h"

namespace stream {

InputManager::InputManager(app::AppContext &context)
    : context(context)
{}

void InputManager::recvRecord(const rapidjson::Document &row) {
    for (rapidjson::Value::ConstMemberIterator it = row.MemberBegin(); it != row.MemberEnd(); ++it) {
        if (!it->value.IsNumber()) { continue; }

        std::string key(it->name.GetString(), it->name.GetStringLength());
        series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *&in = inputs[key];
        if (!in) {
            std::vector<program::ProgObj> args {
                program::ProgObj(key)
            };
            program::ProgObj po = context.get<program::Resolver>().call("input", args);
            series::DataSeries<INPUT_SERIES_ELEMENT_TYPE> *ds = std::get<series::DataSeries<INPUT_SERIES_ELEMENT_TYPE> *>(po);
            in = dynamic_cast<series::InputSeries<INPUT_SERIES_ELEMENT_TYPE> *>(ds);

            context.get<spdlog::logger>().info("Input key " + key);
        }

        in->set(index, static_cast<INPUT_SERIES_ELEMENT_TYPE>(it->value.GetDouble()));
    }

    if (context.has<render::Camera>()) {
        render::Camera &cam = context.get<render::Camera>();
        if (index <= cam.getMax().x && (index + 1) > cam.getMax().x) {
            cam.getMax().x = index + 1;
        }
    }

    index++;
}

void InputManager::end() {
    // Not really much to do here
}

}

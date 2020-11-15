#include "outputmanager.h"

#include <iostream>

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/writer.h"

namespace stream {

void OutputManager::clearEmitters() {
    emitters.clear();
}

void OutputManager::addEmitter(SeriesEmitter *emitter) {
    emitters.push_back(emitter);
}

void OutputManager::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    static thread_local rapidjson::StringBuffer buffer;
    static thread_local rapidjson::Writer<rapidjson::StringBuffer> writer;

    while (true) {
        buffer.Clear();
        writer.Reset(buffer);

        writer.StartObject();

        for (SeriesEmitter *emitter : emitters) {
            std::pair<bool, double> res = emitter->getValue(nextEmitIndex);
            if (!res.first) {
                return;
            }

            writer.Key(emitter->getKey().data(), emitter->getKey().size());
            writer.Double(res.second);
        }

        writer.EndObject();

        std::cout << buffer.GetString() << std::endl;
        std::cout.flush();

        nextEmitIndex++;
    }
}

}

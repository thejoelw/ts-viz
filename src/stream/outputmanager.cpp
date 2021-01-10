#include "outputmanager.h"

#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "app/options.h"
#include "stream/inputmanager.h"

namespace stream {

OutputManager::OutputManager(app::AppContext &context)
    : TickableBase(context)
{}

OutputManager::~OutputManager() {
    emit();
}

void OutputManager::clearEmitters() {
    emitters.clear();
}

void OutputManager::addEmitter(SeriesEmitter *emitter) {
    emitters.push_back(emitter);
}

void OutputManager::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;
    emit();
}

void OutputManager::emit() {
    if (emitters.empty()) {
        return;
    }

    static thread_local rapidjson::StringBuffer buffer;
    static thread_local rapidjson::Writer<rapidjson::StringBuffer> writer;

    while (true) {
        buffer.Clear();
        writer.Reset(buffer);

        writer.StartObject();

        for (SeriesEmitter *emitter : emitters) {
            std::pair<bool, double> res = emitter->getValue(nextEmitIndex);
            if (!res.first) {
                goto finishLoop;
            }

            writer.Key(emitter->getKey().data(), emitter->getKey().size());
            writer.Double(res.second);
        }

        writer.EndObject();

        std::cout << buffer.GetString() << std::endl;
        std::cout.flush();

        nextEmitIndex++;
    }
    finishLoop:;
}

bool OutputManager::isRunning() const {
    assert(nextEmitIndex <= context.get<InputManager>().getIndex());
    return nextEmitIndex < context.get<InputManager>().getIndex() && !emitters.empty();
}

}

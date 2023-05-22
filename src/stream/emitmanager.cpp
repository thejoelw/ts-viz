#include "emitmanager.h"

#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "log.h"
#include "app/options.h"
#include "app/quitexception.h"
#include "stream/inputmanager.h"
#include "series/garbagecollector.h"
#include "series/chunkbase.h"

#include "defs/ENABLE_PMUOI_FLAG.h"

namespace stream {

EmitManager::EmitManager(app::AppContext &context)
    : TickableBase(context)
{
    context.get<InputManager>();
    assert(app::Options::getInstance().enableEmit);
}

EmitManager::~EmitManager() {
    emit();
}

void EmitManager::clearEmitters() {
    curEmitters.clear();
}

void EmitManager::addEmitter(SeriesEmitter *emitter) {
    curEmitters.push_back(emitter);
}

void EmitManager::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    if (ENABLE_PMUOI_FLAG && app::Options::getInstance().printMemoryUsageOutputIndex != static_cast<std::size_t>(-1)) {
        for (SeriesEmitter *emitter : curEmitters) {
            emitter->getValue(app::Options::getInstance().printMemoryUsageOutputIndex);
        }
        SPDLOG_CRITICAL("Initialized all the emitters; memory usage is at {}", context.get<series::GarbageCollector<series::ChunkBase>>().getMemoryUsage());
        throw app::QuitException();
    } else {
        emit();
    }
}

void EmitManager::emit() {
    static thread_local rapidjson::StringBuffer buffer;
    static thread_local rapidjson::Writer<rapidjson::StringBuffer> writer;

    std::size_t endIndex = context.get<InputManager>().getIndex();
    while (nextEmitIndex < endIndex) {
        buffer.Clear();
        writer.Reset(buffer);

        writer.StartObject();

        for (SeriesEmitter *emitter : curEmitters) {
            std::pair<bool, double> res = emitter->getValue(nextEmitIndex);
            if (!res.first) {
                goto finishLoop;
            }

            writer.Key(emitter->getKey().data(), emitter->getKey().size());
            if (std::isfinite(res.second)) {
                writer.Double(res.second);
            } else {
                writer.Null();
            }
        }

        writer.EndObject();

        std::cout << buffer.GetString() << std::endl;
        std::cout.flush();

        nextEmitIndex++;
    }
    finishLoop:;
}

}

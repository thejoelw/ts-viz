#include "emitmanager.h"

#include <iostream>
#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "log.h"
#include "app/options.h"
#include "app/quitexception.h"
#include "series/garbagecollector.h"
#include "series/chunkbase.h"

#include "defs/ENABLE_PMUOI_FLAG.h"

namespace stream {

EmitManager::EmitManager(app::AppContext &context)
    : TickableBase(context)
{
    assert(app::Options::getInstance().emitFormat != app::Options::EmitFormat::None);
}

EmitManager::~EmitManager() {
    emit();
}

void EmitManager::clearEmitters() {
    if (nextEmitIndex != 0 && app::Options::getInstance().emitFormat == app::Options::EmitFormat::Binary) {
        throw std::runtime_error("Cannot modify emitters while writing binary output!");
    }

    curEmitters.clear();
}

void EmitManager::addEmitter(SeriesEmitter *emitter) {
    if (nextEmitIndex != 0 && app::Options::getInstance().emitFormat == app::Options::EmitFormat::Binary) {
        throw std::runtime_error("Cannot modify emitters while writing binary output!");
    }

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
    switch (app::Options::getInstance().emitFormat) {
        case app::Options::EmitFormat::Json: emitJson(); break;
        case app::Options::EmitFormat::Binary: emitBinary(); break;
        default: assert(false);
    }
}

void EmitManager::emitJson() {
    static thread_local rapidjson::StringBuffer buffer;
    static thread_local rapidjson::Writer<rapidjson::StringBuffer> writer;

    while (!curEmitters.empty()) {
        buffer.Clear();
        writer.Reset(buffer);

        writer.StartObject();

        for (SeriesEmitter *emitter : curEmitters) {
            std::pair<bool, double> res = emitter->getValue(nextEmitIndex);
            if (!res.first) {
                return;
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
}

void EmitManager::emitBinary() {
    static thread_local std::vector<double> buffer;

    while (!curEmitters.empty()) {
        buffer.clear();

        for (SeriesEmitter *emitter : curEmitters) {
            std::pair<bool, double> res = emitter->getValue(nextEmitIndex);
            if (!res.first) {
                return;
            }

            buffer.push_back(res.second);
        }

        fwrite(buffer.data(), sizeof(double), buffer.size(), stdout);
        fflush(stdout);

        nextEmitIndex++;
    }

}

}

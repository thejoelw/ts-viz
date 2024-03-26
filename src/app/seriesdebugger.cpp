#include "seriesdebugger.h"

#include <fstream>

#include "defs/ENABLE_CHUNK_DEBUG.h"

#include "app/tickercontext.h"
#include "app/options.h"
#include "series/dataseriesbase.h"
#include "series/garbagecollector.h"
#include "series/chunkbase.h"

namespace {
    void formatBytes(std::ofstream &dst, std::size_t bytes) {
        if (bytes < 1024) {
            dst << bytes << "b";
            return;
        }
        bytes /= 1024;
        if (bytes < 1024) {
            dst << bytes << "kb";
            return;
        }
        bytes /= 1024;
        if (bytes < 1024) {
            dst << bytes << "mb";
            return;
        }
        bytes /= 1024;
        dst << bytes << "gb";
    }
}

namespace app {

SeriesDebugger::SeriesDebugger(AppContext &context)
    : TickableBase(context)
{
    assert(!app::Options::getInstance().debugSeriesToFile.empty());
}

void SeriesDebugger::tick(app::TickerContext &tickerContext) {
#if ENABLE_CHUNK_DEBUG
    std::ofstream file(app::Options::getInstance().debugSeriesToFile);

    std::size_t memoryUsage = tickerContext.getAppContext().get<series::GarbageCollector<series::ChunkBase>>().getMemoryUsage();
    std::size_t memoryLimit = app::Options::getInstance().gcMemoryLimit;
    file << "Memory: ";
    formatBytes(file, memoryUsage);
    file << " / ";
    formatBytes(file, memoryLimit);
    file << std::endl << std::endl;

    for (series::DataSeriesBase *ds : context.get<series::DataSeriesBase::Registry>().registry) {
        ds->writeDebug(file);
    }

    file.close();
#endif
}

}

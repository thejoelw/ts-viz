#include "dataseriesbase.h"

#include <atomic>

#include "jw_util/thread.h"

#include "app/tickercontext.h"

namespace {

template <typename ValueType, typename OpType>
static ValueType atomicApply(std::atomic<ValueType> &atom, OpType op) {
    ValueType prev = atom;
    while (!atom.compare_exchange_weak(prev, op(prev))) {}
    return prev;
}

}

namespace series {

thread_local std::vector<ChunkBase *> DataSeriesBase::dependencyStack;

DataSeriesBase::DataSeriesBase(app::AppContext &context)
    : context(context)
#if ENABLE_CHUNK_MULTITHREADING
    , avgRunDuration(std::chrono::duration<float>::zero())
#endif
{
    class DepStackResetter : public app::TickerContext::TickableBase<DepStackResetter> {
    public:
        DepStackResetter(app::AppContext &context)
            : TickableBase(context)
        {}

        void tick(app::TickerContext &tickerContext) {
            (void) tickerContext;

            getDependencyStack().clear();
        }
    };

    context.get<DepStackResetter>();
}

#if ENABLE_CHUNK_MULTITHREADING
void DataSeriesBase::recordDuration(std::chrono::duration<float> duration) {
    atomicApply(avgRunDuration, [duration](std::chrono::duration<float> ard) {
        static constexpr float durationSampleResponse = 0.1f;
        ard *= 1.0f - durationSampleResponse;
        ard += duration * durationSampleResponse;
        return ard;
    });
}

std::chrono::duration<float> DataSeriesBase::getAvgRunDuration() const {
    return avgRunDuration.load();
}
#endif

std::vector<ChunkBase *> &DataSeriesBase::getDependencyStack() {
    jw_util::Thread::assert_main_thread();

    return dependencyStack;
}

}

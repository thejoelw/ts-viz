#include "dataseriesbase.h"

template <typename ValueType, typename OpType>
static ValueType atomicApply(std::atomic<ValueType> &atom, OpType op) {
    ValueType prev = atom;
    while (!atom.compare_exchange_weak(prev, op(prev))) {}
    return prev;
}

namespace series {

thread_local std::vector<ChunkBase *> DataSeriesBase::dependencyStack;

DataSeriesBase::DataSeriesBase(app::AppContext &context)
    : context(context)
    , avgRunDuration(std::chrono::duration<float>::zero())
{}

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

}

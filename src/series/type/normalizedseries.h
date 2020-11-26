#pragma once

#include "series/dataseries.h"
#include "series/invalidparameterexception.h"

namespace series {

template <typename ElementType, typename ArgType>
class NormalizedSeries : public DataSeries<ElementType> {
public:
    NormalizedSeries(app::AppContext &context, ArgType &arg, std::size_t normSize, bool zeroOutside)
        : DataSeries<ElementType>(context)
        , arg(arg)
        , normSize(normSize)
        , zeroOutside(zeroOutside)
    {
        if (normSize == 0) {
            throw series::InvalidParameterException("NormalizedSeries::normSize must be greater than zero");
        }

        if (zeroOutside) {
            throw series::InvalidParameterException("NormalizedSeries::zeroOutside being true is not implemented yet");
        }
    }

    Chunk<ElementType> *makeChunk(std::size_t chunkIndex) override {
        std::vector<ChunkPtr<ElementType>> normChunks;
        for (std::size_t i = 0; i < normSize; i += CHUNK_SIZE) {
            normChunks.push_back(arg.getChunk(i / CHUNK_SIZE));
        }

        ChunkPtr<ElementType> srcChunk = arg.getChunk(chunkIndex);

        return this->constructChunk([this, normChunks = std::move(normChunks), srcChunk = std::move(srcChunk)](ElementType *dst, unsigned int computedCount) -> unsigned int {
            if (std::isnan(factor)) {
                for (std::size_t i = 0; i < normChunks.size(); i++) {
                    unsigned int minCount = std::min(i * CHUNK_SIZE, normSize);
                    if (normChunks->getComputedCount() < minCount) {
                        assert(computedCount == 0);
                        return 0;
                    }
                }

                double sum = 0.0;
                // Iterate backwards so we hopefully sum the smallest elements first
                for (std::size_t i = normSize; i-- > 0;) {
                    sum += normChunks[i / CHUNK_SIZE]->getElement(i % CHUNK_SIZE);
                }
                factor = 1.0 / sum;

                if (std::isnan(factor)) {
                    throw jw_util::BaseException("Normalization multiplier is NaN");
                }
            }
            double a = factor;

            unsigned int endCount = srcChunk->getComputedCount();
            while (computedCount < endCount) {
                dst[computedCount] = srcChunk->getElement(computedCount) * a;
                computedCount++;
            }

            return endCount;
        });
    }

private:
    ArgType &arg;

    std::size_t normSize;
    bool zeroOutside;
    std::atomic<double> factor = NAN;
};

}

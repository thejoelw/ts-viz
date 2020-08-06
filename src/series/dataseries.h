#pragma once

#include <vector>

#include "series/series.h"
#include "render/seriesrenderer.h"

namespace series {

template <typename ElementType>
class DataSeries : public Series {
public:
    DataSeries(app::AppContext &context)
        : Series(context)
    {}

    ~DataSeries() {
        delete[] renderer;
    }

    void draw(std::size_t begin, std::size_t end, std::size_t stride) override {
        assert(begin <= end);

        if (!renderer) {
            renderer = new render::SeriesRenderer<ElementType>(context);
        }

        if (begin < computedBegin) {
            begin = computedBegin;
        }
        if (end > computedEnd) {
            end = computedEnd;
        }

        assert(begin <= end);
        if (begin == end) {
            return;
        }

        static thread_local std::vector<ElementType> sample;
        sample.clear();
        sample.reserve((end - begin + stride - 1) / stride);

        std::size_t storedBegin = getStoredBegin();
        for (std::size_t i = begin; i < end; i += stride) {
            sample.push_back(data[i - storedBegin]);
        }

        renderer->draw(begin, stride, sample);
    }

    void compute(std::size_t begin, std::size_t end) override {
        assert(begin < end);
        assert(begin <= computedBegin);
        assert(end >= computedEnd);

        if (begin == computedBegin && end == computedEnd) {
            return;
        }

        std::size_t prevStoreBegin = getStoredBegin();
        std::size_t prevStoreEnd = getStoredEnd();
        std::size_t prevBegin = computedBegin;
        std::size_t prevEnd = computedEnd;
        computedBegin = begin;
        computedEnd = end;
        std::size_t newStoreBegin = getStoredBegin();
        std::size_t newStoreEnd = getStoredEnd();

        if (newStoreBegin != prevStoreBegin || newStoreEnd != prevStoreEnd) {
            ElementType *newData = new ElementType[newStoreEnd - newStoreBegin];
            if (data) {
                std::copy(data + (prevBegin - prevStoreBegin), data + (prevEnd - prevStoreBegin), newData + (prevBegin - newStoreBegin));
                delete[] data;
            }
            data = newData;
        }

        if ((prevEnd - prevBegin) * 3 > end - begin) {
            if (begin < prevBegin) {
                computeInto(data + (begin - newStoreBegin), begin, prevBegin);
            }
            if (end > prevEnd) {
                computeInto(data + (prevEnd - newStoreBegin), prevEnd, end);
            }
        } else {
            computeInto(data + (begin - newStoreBegin), begin, end);
        }
    }
    virtual void computeInto(ElementType *dst, std::size_t begin, std::size_t end) = 0;

    std::pair<ElementType *, ElementType *> getData(std::size_t begin, std::size_t end) {
        assert(begin >= computedBegin);
        assert(end <= computedEnd);
        std::size_t storedBegin = getStoredBegin();
        return std::make_pair(data + (begin - storedBegin), data + (end - storedBegin));
    }

private:
    ElementType *data = 0;
    render::SeriesRenderer<ElementType> *renderer = 0;

    unsigned int getPaddingSizeLog2() const {
        std::size_t size = computedEnd - computedBegin;
        return (sizeof(unsigned long long) * CHAR_BIT - 1) - __builtin_clzll((size / 4) | 1);
    }
    std::size_t getStoredBegin() const {
        unsigned int paddingSizeLog2 = getPaddingSizeLog2();
        return (computedBegin >> paddingSizeLog2) << paddingSizeLog2;
    }
    std::size_t getStoredEnd() const {
        unsigned int paddingSizeLog2 = getPaddingSizeLog2();
        return (((computedEnd - 1) >> paddingSizeLog2) + 1) << paddingSizeLog2;
    }
};

}

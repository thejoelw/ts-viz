#pragma once

#include <vector>
#include <fftw3.h>

#include "series/series.h"
#include "render/seriesrenderer.h"

namespace series {

template <typename ElementType>
class DataSeries : public Series {
public:
    class DynamicWidthException : public jw_util::BaseException {
        friend class DataSeries<ElementType>;

    private:
        DynamicWidthException(const std::string &msg)
            : BaseException(msg)
        {}
    };

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

        if (begin >= end) {
            return;
        }

        static thread_local std::vector<ElementType> sample;
        sample.clear();
        sample.reserve((end - begin + stride - 1) / stride);

        for (std::size_t i = begin; i < end; i += stride) {
            sample.push_back(data[i - allocatedBegin]);
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

        reallocData();

        if ((computedEnd - computedBegin) * 3 > end - begin) {
            if (begin < computedBegin) {
                std::pair<std::size_t, std::size_t> res = computeInto(data + (begin - allocatedBegin), begin, computedBegin);
                if (res.first != res.second) {
                    assert(res.first < res.second);
                    assert(res.second == computedBegin);
                    computedBegin = res.first;
                }
            }
            if (end > computedEnd) {
                std::pair<std::size_t, std::size_t> res = computeInto(data + (computedEnd - allocatedBegin), computedEnd, end);
                if (res.first != res.second) {
                    assert(res.first < res.second);
                    assert(res.first == computedEnd);
                    computedEnd = res.second;
                }
            }
        } else {
            std::pair<std::size_t, std::size_t> res = computeInto(data + (begin - allocatedBegin), begin, end);
            if (res.first != res.second) {
                assert(res.first < res.second);
                if (res.first < computedBegin) {
                    computedBegin = res.first;
                }
                if (res.second > computedEnd) {
                    computedEnd = res.second;
                }
            }
        }
    }
    virtual std::pair<std::size_t, std::size_t> computeInto(ElementType *dst, std::size_t begin, std::size_t end) = 0;

    std::pair<ElementType *, ElementType *> getData(std::size_t begin, std::size_t end) {
        assert(begin >= computedBegin);
        assert(end <= computedEnd);
        return std::make_pair(data + (begin - allocatedBegin), data + (end - allocatedBegin));
    }

    virtual std::size_t getStaticWidth() const {
        throw DynamicWidthException("Cannot get static width for series");
    }

    std::size_t getAllocatedBegin() const { return allocatedBegin; }
    std::size_t getAllocatedEnd() const { return allocatedEnd; }

private:
    ElementType *data = 0;

    std::size_t allocatedBegin = 0;
    std::size_t allocatedEnd = 0;

    render::SeriesRenderer<ElementType> *renderer = 0;

    void reallocData() {
        std::size_t size = requestedEnd - requestedBegin;
        unsigned int minSizeLog2 = (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(size | 1);
        std::size_t newBegin = (requestedBegin >> minSizeLog2) << minSizeLog2;
        std::size_t newEnd = (((requestedEnd - 1) >> minSizeLog2) + 1) << minSizeLog2;

        size = newEnd - newBegin;
        assert((size & (size - 1)) == 0);

        assert(newBegin <= allocatedBegin);
        assert(newEnd >= allocatedEnd);
        if (newBegin == allocatedBegin && newEnd == allocatedEnd) {
            return;
        }

        ElementType *newData = static_cast<ElementType *>(fftw_malloc(sizeof(ElementType) * (newEnd - newBegin)));
        if (data) {
            ElementType *b1 = newData + (computedBegin - newBegin);
            ElementType *b2 = newData + (computedEnd - newBegin);
            ElementType *b3 = newData + (newEnd - newBegin);
            std::fill(newData, b1, static_cast<ElementType>(0.0));
            std::copy(data + (computedBegin - allocatedBegin), data + (computedEnd - allocatedBegin), b1);
            std::fill(b2, b3, static_cast<ElementType>(0.0));
            fftw_free(data);
        }
        data = newData;
        allocatedBegin = newBegin;
        allocatedEnd = newEnd;
    }
};

}

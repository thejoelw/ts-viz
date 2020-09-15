#pragma once

#include "series/dataseries.h"

namespace series {

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    static constexpr std::size_t fftSizeThreshold = 32;

public:
    ConvSeries(app::AppContext &context, const DataSeries<ElementType> &kernel, const DataSeries<ElementType> &ts)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelWidth(kernel.getStaticWidth())
    {
        assert(kernelWidth > 0);

        this->dependsOn(&kernel);
        this->dependsOn(&ts);
    }

    void propogateRequest() override {
        kernel.request(0, kernelWidth);
        std::size_t sub = kernelWidth - 1;
        ts.request(this->requestedBegin > sub ? this->requestedBegin - sub : 0, this->requestedEnd);
    }

    std::pair<std::size_t, std::size_t> computeInto(ElementType *dst, std::size_t begin, std::size_t end) override {
        if (kernel.getComputedBegin() == kernel.getComputedEnd()) {
            return std::make_pair(0, 0);
        }

        assert(kernel.getComputedBegin() == 0);
        assert(kernel.getComputedEnd() == kernelWidth);

        std::size_t minBegin = ts.getComputedBegin() + kernelWidth - 1;
        if (begin < minBegin) { begin = minBegin; }

        std::size_t maxEnd = ts.getComputedEnd();
        if (end > maxEnd) { end = maxEnd; }

        if (begin >= end) {
            return std::make_pair(0, 0);
        }

        std::size_t size = end - begin;
        if (size < fftSizeThreshold) {
            std::pair<ElementType *, ElementType *> kernelRange = kernel.getData(0, kernelWidth);
            std::pair<ElementType *, ElementType *> tsRange = ts.getData(begin, end);

            for (ElementType *tsIt = tsRange.first; tsIt < tsRange.second; tsIt++) {
                ElementType *tsIt2 = tsIt;
                ElementType sum = 0.0;
                // TODO: Sum from smallest to biggest
                for (ElementType *kIt = kernelRange.first; kIt < kernelRange.second; kIt++) {
                    sum += *kIt * *tsIt2--;
                }
                *dst++ = sum;
            }
        } else {
            assert(size > 1);
            std::size_t ceilLog2 = (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(size - 1);
            std::size_t nextPow2 = static_cast<std::size_t>(1) << ceilLog2;

            if (planSize != nextPow2) {
                assert(nextPow2 >= size);
                std::size_t remainingPadding = nextPow2 - size;

                std::size_t expandEnd = std::min(remainingPadding, ts.getAllocatedEnd() - end);
                remainingPadding -= expandEnd;
                std::size_t end2 = end + expandEnd;

                std::size_t expandBegin = std::min(remainingPadding, begin - ts.getAllocatedBegin());
                remainingPadding -= expandBegin;
                std::size_t begin2 = begin - expandBegin;

                assert(remainingPadding == 0);
                assert(end2 - begin2 == nextPow2);

                planSize = nextPow2;
                out = fftw_alloc_complex(nextPow2);
                plan = fftw_plan_dft_r2c_1d(nextPow2, ts.getData(begin2, end2), out, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
            }

            fftw_execute


            fftw_complex in[N];
            fftw_complex out[N];
            fftw_plan p = fftw_create_plan(nextPow2, FFTW_FORWARD, FFTW_MEASURE);
            fftw_one(p, in, out);
            fftw_destroy_plan(p);
        }


        begin = std::apply([begin](auto &... x){return vmax(begin, x.getComputedBegin()...);}, args);
        end = std::apply([end](auto &... x){return vmin(end, x.getComputedEnd()...);}, args);
        if (begin >= end) {
            return std::make_pair(0, 0);
        }
        auto sources = std::apply([begin, end](auto &... x){return std::make_tuple(x.getData(begin, end).first...);}, args);

        for (std::size_t i = begin; i < end; i++) {
            *dst++ = std::apply([this](auto *&... s){return op(*s++...);}, sources);
        }

        return std::make_pair(begin, end);
    }

private:
    const DataSeries<ElementType> &kernel;
    const DataSeries<ElementType> &ts;

    std::size_t kernelWidth;

    std::size_t planSize = 0;
    fftw_complex *out;
    fftw_plan plan;
};

}

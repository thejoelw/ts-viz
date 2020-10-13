#pragma once

#include <fftw3.h>

#include "series/dataseries.h"
#include "series/finitecompseries.h"

namespace {

static unsigned long long nextPow2(unsigned long long x) {
    if (x < 2) {
        return 1;
    }
    unsigned int ceilLog2 = (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(x - 1);
    return static_cast<unsigned long long>(1) << ceilLog2;
}

template <typename ElementType>
struct fftwx_impl;

template<> struct fftwx_impl<float> {
    typedef fftwf_plan Plan;
    typedef fftwf_complex Complex;

    static float *alloc_real(std::size_t size) { return fftwf_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return fftwf_alloc_complex(size); }
    static void free(void *ptr) { return fftwf_free(ptr); }

    static Plan plan_dft_r2c_1d(std::size_t size, float *in, Complex *out, unsigned flags) { return fftwf_plan_dft_r2c_1d(size, in, out, flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, float *out, unsigned flags) { return fftwf_plan_dft_c2r_1d(size, in, out, flags); }

    static void execute(Plan plan) { fftwf_execute(plan); }
    static void execute_dft_r2c(Plan plan, float *in, Complex *out) { fftwf_execute_dft_r2c(plan, in, out); }
    static void execute_dft_c2r(Plan plan, Complex *in, float *out) { fftwf_execute_dft_c2r(plan, in, out); }

    static void destroy_plan(Plan plan) { fftwf_destroy_plan(plan); }
};

template<> struct fftwx_impl<double> {
    typedef fftw_plan Plan;
    typedef fftw_complex Complex;

    static double *alloc_real(std::size_t size) { return fftw_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return fftw_alloc_complex(size); }
    static void free(void *ptr) { return fftw_free(ptr); }

    static Plan plan_dft_r2c_1d(std::size_t size, double *in, Complex *out, unsigned flags) { return fftw_plan_dft_r2c_1d(size, in, out, flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, double *out, unsigned flags) { return fftw_plan_dft_c2r_1d(size, in, out, flags); }

    static void execute(Plan plan) { fftw_execute(plan); }
    static void execute_dft_r2c(Plan plan, double *in, Complex *out) { fftw_execute_dft_r2c(plan, in, out); }
    static void execute_dft_c2r(Plan plan, Complex *in, double *out) { fftw_execute_dft_c2r(plan, in, out); }

    static void destroy_plan(Plan plan) { fftw_destroy_plan(plan); }
};

}

namespace series {

extern std::mutex fftwMutex;

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    static constexpr std::size_t fftSizeThreshold = 32;

    static constexpr std::size_t chunkSize = DataSeries<ElementType>::Chunk::size;

    typedef fftwx_impl<ElementType> fftwx;

public:
    ConvSeries(app::AppContext &context, FiniteCompSeries<ElementType> &kernel, DataSeries<ElementType> &ts)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelBack(kernel.getSize() - 1)
        , sourceSize(kernelBack + chunkSize)
        , planSize(nextPow2(sourceSize))
    {
        assert(kernelBack != static_cast<std::size_t>(-1));
    }

    ~ConvSeries() {
        std::unique_lock<std::mutex> lock(fftwMutex);

        for (PlanIO planIO : planIOs) {
            fftwx::free(planIO.in);
            fftwx::free(planIO.fft);
            fftwx::free(planIO.out);
        }

        fftwx::free(kernelFft);

        fftwx::destroy_plan(planBwd);
        fftwx::destroy_plan(planFwd);
    }

    std::function<void(ElementType *)> getChunkGenerator(std::size_t chunkIndex) override {
        std::size_t begin = chunkIndex * chunkSize;
        std::size_t end = (chunkIndex + 1) * chunkSize;
        if (end <= kernelBack) {
            return [](ElementType *dst) {
                std::fill_n(dst, chunkSize, NAN);
            };
        }

        std::size_t numTsChunks = std::min((sourceSize - 1) / chunkSize, chunkIndex) + 1;
        typename DataSeries<ElementType>::Chunk **tsChunks = new typename DataSeries<ElementType>::Chunk *[numTsChunks];
        for (std::size_t i = 0; i < numTsChunks; i++) {
            tsChunks[i] = ts.getChunk(chunkIndex - numTsChunks + i + 1);
        }

        return [this, begin, end, numTsChunks, tsChunks](ElementType *dst) {
            PlanIO planIO = requestPlanIO();

            assert(numTsChunks <= planSize / chunkSize);
            assert(numTsChunks * chunkSize <= planSize);
            for (std::size_t i = 0; i < numTsChunks; i++) {
                std::copy_n(tsChunks[i]->getData(), chunkSize, planIO.in + planSize - (numTsChunks - i) * chunkSize);
            }
            std::fill(planIO.in, planIO.in + planSize - std::min(sourceSize, numTsChunks * chunkSize), static_cast<ElementType>(0.0));

            fftwx::execute_dft_r2c(planFwd, planIO.in, planIO.fft);

            // Elementwise multiply
            for (std::size_t i = 0; i < planSize; i++) {
                ElementType *a = planIO.fft[i];
                ElementType *b = kernelFft[i];

                ElementType c = a[0] * b[0] - a[1] * b[1];
                ElementType d = a[0] * b[1] + a[1] * b[0];
                a[0] = c;
                a[1] = d;
            }

            // Backward fft
            fftwx::execute_dft_c2r(planBwd, planIO.fft, planIO.out);

            std::copy_n(planIO.out + planSize - chunkSize, chunkSize, dst);

            releasePlanIO(planIO);

            delete[] tsChunks;

            assert(kernelBack < end);
            if (begin < kernelBack) {
                std::fill_n(dst, kernelBack - begin, NAN);
            }
        };

    }

private:
    FiniteCompSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    std::size_t kernelBack;
    std::size_t sourceSize;
    std::size_t planSize;

    bool hasPlan = false;
    typename fftwx::Plan planFwd;
    typename fftwx::Plan planBwd;

    typename fftwx::Complex *kernelFft;

    struct PlanIO {
        ElementType *in;
        typename fftwx::Complex *fft;
        ElementType *out;
    };
    std::vector<PlanIO> planIOs;

    PlanIO requestPlanIO() {
        std::unique_lock<std::mutex> lock(fftwMutex);
        if (!hasPlan) {
            processKernel();
            hasPlan = true;
        }

        return makePlanIO();
    }

    PlanIO makePlanIO() {
        if (planIOs.empty()) {
            PlanIO res;
            res.in = fftwx::alloc_real(planSize);
            res.fft = fftwx::alloc_complex(planSize);
            res.out = fftwx::alloc_real(planSize);
            return res;
        } else {
            PlanIO res = planIOs.back();
            planIOs.pop_back();
            return res;
        }
    }

    void releasePlanIO(PlanIO planIO) {
        std::unique_lock<std::mutex> lock(fftwMutex);
        planIOs.push_back(planIO);
    }

    void processKernel() {
        ElementType *tmpIn = fftwx::alloc_real(planSize);
        typename fftwx::Complex *tmpFft = fftwx::alloc_complex(planSize);

        planFwd = fftwx::plan_dft_r2c_1d(planSize, tmpIn, tmpFft, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        planBwd = fftwx::plan_dft_c2r_1d(planSize, tmpFft, tmpIn, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);

        ElementType invPlanSize = static_cast<ElementType>(1.0) / planSize;
        for (std::size_t i = 0; i < kernel.getSize(); i++) {
            tmpIn[i] = kernel.getData()[i] * invPlanSize;
        }
        std::fill(tmpIn + kernelBack + 1, tmpIn + planSize, static_cast<ElementType>(0.0));
        fftwx::execute(planFwd);

        fftwx::free(tmpIn);
        kernelFft = tmpFft;
    }
};

}

#pragma once

#include <fftw3.h>
#include <complex>

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

    static std::complex<float> getComplex(Complex src) { return std::complex<float>(src[0], src[1]); }
    static void setComplex(Complex dst, std::complex<float> src) { dst[0] = src.real(); dst[1] = src.imag(); }
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

    static std::complex<double> getComplex(Complex src) { return std::complex<double>(src[0], src[1]); }
    static void setComplex(Complex dst, std::complex<double> src) { dst[0] = src.real(); dst[1] = src.imag(); }
};

}

namespace series {

extern std::mutex fftwMutex;

template <typename ElementType>
class ConvSeries : public DataSeries<ElementType> {
private:
    static constexpr std::size_t fftSizeThreshold = 32;

    typedef fftwx_impl<ElementType> fftwx;

public:
    ConvSeries(app::AppContext &context, FiniteCompSeries<ElementType> &kernel, DataSeries<ElementType> &ts, ElementType derivativeOrder, bool backfillZeros)
        : DataSeries<ElementType>(context)
        , kernel(kernel)
        , ts(ts)
        , kernelBack(kernel.getSize() - 1)
        , sourceSize(kernelBack + CHUNK_SIZE)
        , planSize(nextPow2(sourceSize))
        , derivativeOrder(derivativeOrder)
        , backfillZeros(backfillZeros)
    {
        assert(kernel.getSize() > 0);
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
        typedef typename DataSeries<ElementType>::Chunk Chunk;

        std::size_t begin = chunkIndex * CHUNK_SIZE;
        std::size_t end = (chunkIndex + 1) * CHUNK_SIZE;
        if (!backfillZeros && end <= kernelBack) {
            return [](ElementType *dst) {
                std::fill_n(dst, CHUNK_SIZE, NAN);
            };
        }

        std::size_t numTsChunks = std::min((sourceSize - 1) / CHUNK_SIZE, chunkIndex) + 1;
        std::shared_ptr<Chunk> *tsChunks = new std::shared_ptr<Chunk>[numTsChunks];
        for (std::size_t i = 0; i < numTsChunks; i++) {
            tsChunks[i] = ts.getChunk(chunkIndex - numTsChunks + i + 1);
        }

        return [this, begin, end, numTsChunks, tsChunks](ElementType *dst) {
            PlanIO planIO = requestPlanIO();

            static_assert(sizeof(signed int) * CHAR_BIT > CHUNK_SIZE_LOG2, "CHUNK_SIZE_LOG2 is too big");
            static thread_local std::vector<std::pair<signed int, signed int>> nans;
            nans.clear();
            nans.push_back(std::pair<signed int, signed int>(INT_MIN, INT_MIN));

            assert(numTsChunks <= planSize / CHUNK_SIZE);
            assert(numTsChunks * CHUNK_SIZE <= planSize);
            for (std::size_t i = 0; i < numTsChunks; i++) {
                ElementType *src = tsChunks[i]->getData();
                ElementType *dst = planIO.in + planSize - (numTsChunks - i) * CHUNK_SIZE;
                for (std::size_t j = 0; j < CHUNK_SIZE; j++) {
                    ElementType val = src[j];
                    if (std::isnan(val)) {
                        val = 0.0;

                        signed int rangeBegin = j - (numTsChunks - i) * CHUNK_SIZE;
                        signed int rangeEnd = rangeBegin + (kernelBack + 1);
                        if (rangeBegin <= nans.back().second) {
                            nans.back().second = rangeEnd;
                        } else {
                            nans.push_back(std::pair<signed int, signed int>(rangeBegin, rangeEnd));
                        }
                    }
                    dst[j] = val;
                }
            }

            delete[] tsChunks;

            std::fill(planIO.in, planIO.in + planSize - std::min(sourceSize, numTsChunks * CHUNK_SIZE), static_cast<ElementType>(0.0));

            fftwx::execute_dft_r2c(planFwd, planIO.in, planIO.fft);

            // Elementwise multiply
            for (std::size_t i = 0; i < planSize; i++) {
                fftwx::setComplex(planIO.fft[i], fftwx::getComplex(planIO.fft[i]) * fftwx::getComplex(kernelFft[i]));
            }

            // Backward fft
            fftwx::execute_dft_c2r(planBwd, planIO.fft, planIO.out);

            std::copy_n(planIO.out + planSize - CHUNK_SIZE, CHUNK_SIZE, dst);

            releasePlanIO(planIO);

            if (!backfillZeros && begin < kernelBack) {
                std::fill_n(dst, std::min(kernelBack, end) - begin, NAN);
            }

            if (!backfillZeros) {
                for (std::pair<signed int, signed int> range : nans) {
                    static constexpr signed int signedChunkSize = CHUNK_SIZE;
                    range.first = std::max(0, signedChunkSize + range.first);
                    range.second = std::min(signedChunkSize, signedChunkSize + range.second);
                    if (range.first < range.second) {
                        std::fill(dst + range.first, dst + range.second, NAN);
                    }
                }
            }
        };

    }

private:
    FiniteCompSeries<ElementType> &kernel;
    DataSeries<ElementType> &ts;

    std::size_t kernelBack;
    std::size_t sourceSize;
    std::size_t planSize;
    ElementType derivativeOrder;
    bool backfillZeros;

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

        if (derivativeOrder != 0) {
            for (std::size_t i = 0; i < planSize; i++) {
                std::complex<ElementType> z(0.0, static_cast<ElementType>(i) / planSize);
                std::complex<ElementType> f = std::pow(z, derivativeOrder);
                fftwx::setComplex(tmpFft[i], fftwx::getComplex(tmpFft[i]) * f);
            }
        }

        kernelFft = tmpFft;
    }
};

}

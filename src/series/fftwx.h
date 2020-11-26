#pragma once

#include <complex>
#include <vector>
#include <array>

#include <fftw3.h>

#include "jw_util/thread.h"

#include "series/chunksize.h"

namespace {

template <std::size_t x>
constexpr unsigned int exactLog2() {
    static_assert(x % 2 == 0, "Non-power-of-2");
    return x == 1 ? 0 : 1 + exactLog2<x / 2>();
}

}

namespace series {

template <typename ElementType>
struct fftwx_impl;

template<> struct fftwx_impl<float> {
    typedef fftwf_plan Plan;
    typedef fftwf_complex Complex;

    static float *alloc_real(std::size_t size) { return fftwf_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return fftwf_alloc_complex(size); }
    static void free(void *ptr) { return fftwf_free(ptr); }
    static int alignment_of(float *p) { return fftwf_alignment_of(p); }

    static Plan plan_dft_r2c_1d(std::size_t size, float *in, Complex *out, unsigned flags) { return fftwf_plan_dft_r2c_1d(size, in, out, flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, float *out, unsigned flags) { return fftwf_plan_dft_c2r_1d(size, in, out, flags); }

    static void execute(Plan plan) { fftwf_execute(plan); }
    static void execute_dft_r2c(Plan plan, float *in, Complex *out) { fftwf_execute_dft_r2c(plan, in, out); }
    static void execute_dft_c2r(Plan plan, Complex *in, float *out) { fftwf_execute_dft_c2r(plan, in, out); }

    static void destroy_plan(Plan plan) { fftwf_destroy_plan(plan); }
    static void cleanup() { fftwf_cleanup(); }

    static std::complex<float> getComplex(Complex src) { return std::complex<float>(src[0], src[1]); }
    static void setComplex(Complex dst, std::complex<float> src) { dst[0] = src.real(); dst[1] = src.imag(); }
};

template<> struct fftwx_impl<double> {
    typedef fftw_plan Plan;
    typedef fftw_complex Complex;

    static double *alloc_real(std::size_t size) { return fftw_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return fftw_alloc_complex(size); }
    static void free(void *ptr) { return fftw_free(ptr); }
    static int alignment_of(double *p) { return fftw_alignment_of(p); }

    static Plan plan_dft_r2c_1d(std::size_t size, double *in, Complex *out, unsigned flags) { return fftw_plan_dft_r2c_1d(size, in, out, flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, double *out, unsigned flags) { return fftw_plan_dft_c2r_1d(size, in, out, flags); }

    static void execute(Plan plan) { fftw_execute(plan); }
    static void execute_dft_r2c(Plan plan, double *in, Complex *out) { fftw_execute_dft_r2c(plan, in, out); }
    static void execute_dft_c2r(Plan plan, Complex *in, double *out) { fftw_execute_dft_c2r(plan, in, out); }

    static void destroy_plan(Plan plan) { fftw_destroy_plan(plan); }
    static void cleanup() { fftw_cleanup(); }

    static std::complex<double> getComplex(Complex src) { return std::complex<double>(src[0], src[1]); }
    static void setComplex(Complex dst, std::complex<double> src) { dst[0] = src.real(); dst[1] = src.imag(); }
};

extern std::mutex fftwMutex;

template <typename ElementType>
class FftwPlanner {
    typedef fftwx_impl<ElementType> fftwx;

public:
    struct IO {
        ElementType *in;
        typename fftwx::Complex *fft;
        ElementType *out; // TODO: Do we really only need this, or just a real array and a complex array?
    };

    FftwPlanner() = delete;

    static_assert(sizeof(typename fftwx::Plan) == sizeof(void *), "Unexpected sizeof(fftwx::Plan)");

    template <std::size_t planSize>
    static typename fftwx::Plan getPlanFwd() {
        assert(isInit);
        static constexpr unsigned int planIndex = exactLog2<planSize>();
        static_assert(planIndex < planFwds.size());
        return planFwds[planIndex];
    }

    template <std::size_t planSize>
    static typename fftwx::Plan getPlanBwd() {
        assert(isInit);
        static constexpr unsigned int planIndex = exactLog2<planSize>();
        static_assert(planIndex < planBwds.size());
        return planBwds[planIndex];
    }

    static const IO request() {
        // TODO: Just allocate these on the stack?
        // TODO: Or even better, just allocate in a static thread_local variable?

        std::unique_lock<std::mutex> lock(fftwMutex);

        IO res;
        if (ios.empty()) {
            res.in = fftwx::alloc_real(CHUNK_SIZE * 2);
            res.fft = fftwx::alloc_complex(CHUNK_SIZE * 2);
            res.out = fftwx::alloc_real(CHUNK_SIZE * 2);
        } else {
            res = ios.back();
            ios.pop_back();
        }

        return res;
    }

    static void release(const IO io) {
        std::unique_lock<std::mutex> lock(fftwMutex);
        assert(std::find(ios.cbegin(), ios.cend(), io) == ios.cend());
        ios.emplace_back(io);
    }

    static void checkAlignment(ElementType *ptr) {
#ifndef NDEBUG
        static thread_local ElementType *authentic = fftwx::alloc_real(1);
        assert(fftwx::alignment_of(ptr) == fftwx::alignment_of(authentic));
#endif
    }
    static void checkAlignment(typename fftwx::Complex *ptr) {
#ifndef NDEBUG
        static thread_local typename fftwx::Complex *authentic = fftwx::alloc_complex(1);
        assert(fftwx::alignment_of(ptr) == fftwx::alignment_of(authentic));
#endif
    }

    static void init() {
        jw_util::Thread::assert_main_thread();

        if (isInit) {
            return;
        }
        isInit = true;

        IO io = request();
        for (unsigned int i = 0; i < planFwds.size(); i++) {
            planFwds[i] = fftwx::plan_dft_r2c_1d(1u << i, io.in, io.fft, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        }
        for (unsigned int i = 0; i < planBwds.size(); i++) {
            planBwds[i] = fftwx::plan_dft_c2r_1d(1u << i, io.fft, io.out, FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
        }
        release(io);
    }

    static void cleanup() {
        jw_util::Thread::assert_main_thread();

        if (!isInit) {
            return;
        }
        isInit = false;

        for (typename fftwx::Plan plan : planFwds) {
            fftwx::destroy_plan(plan);
        }
        for (typename fftwx::Plan plan : planBwds) {
            fftwx::destroy_plan(plan);
        }

        for (IO io : ios) {
            fftwx::free(io.in);
            fftwx::free(io.fft);
        }
        ios.clear();

        fftwx::cleanup();
    }

private:
    inline static bool isInit;
    inline static std::vector<IO> ios;
    inline static std::array<typename fftwx::Plan, CHUNK_SIZE_LOG2 + 1 /* this will probably have to become 2 */> planFwds;
    inline static std::array<typename fftwx::Plan, CHUNK_SIZE_LOG2 + 1> planBwds;
};

}

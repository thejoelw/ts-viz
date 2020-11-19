#pragma once

#include <complex>

#include <fftw3.h>

namespace series {

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

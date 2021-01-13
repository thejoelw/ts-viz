#pragma once

#include <complex>
#include <vector>
#include <array>
#include <mutex>

#include <fftw3.h>

#include "log.h"

#include "jw_util/thread.h"
#include "jw_util/typename.h"
#include "jw_util/baseexception.h"

#include "app/options.h"

#include "series/chunksize.h"

#include "defs/FFTWX_PLANNING_LEVEL.h"

namespace {

template <std::size_t x>
constexpr unsigned int exactLog2() {
    static_assert(x == 1 || x % 2 == 0, "Non-power-of-2");
    return x == 1 ? 0 : 1 + exactLog2<x / 2>();
}

}

namespace series {

template <typename ElementType>
struct fftwx_impl;

template<> struct fftwx_impl<float> {
    typedef fftwf_plan Plan;
    typedef std::complex<float> Complex;

    static float *alloc_real(std::size_t size) { return fftwf_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return reinterpret_cast<Complex *>(fftwf_alloc_complex(size)); }
    static void free(void *ptr) { return fftwf_free(ptr); }
    static int alignment_of(float *p) { return fftwf_alignment_of(p); }

    static int import_wisdom_from_filename(const char *filename) { return fftwf_import_wisdom_from_filename(filename); }
    static int export_wisdom_to_filename(const char *filename) { return fftwf_export_wisdom_to_filename(filename); }

    static Plan plan_dft_r2c_1d(std::size_t size, float *in, Complex *out, unsigned flags) { return fftwf_plan_dft_r2c_1d(size, in, reinterpret_cast<fftwf_complex *>(out), flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, float *out, unsigned flags) { return fftwf_plan_dft_c2r_1d(size, reinterpret_cast<fftwf_complex *>(in), out, flags); }

    static void execute(Plan plan) { fftwf_execute(plan); }
    static void execute_dft_r2c(Plan plan, float *in, Complex *out) { fftwf_execute_dft_r2c(plan, in, reinterpret_cast<fftwf_complex *>(out)); }
    static void execute_dft_c2r(Plan plan, Complex *in, float *out) { fftwf_execute_dft_c2r(plan, reinterpret_cast<fftwf_complex *>(in), out); }

    static void destroy_plan(Plan plan) { fftwf_destroy_plan(plan); }
    static void cleanup() { fftwf_cleanup(); }
};

template<> struct fftwx_impl<double> {
    typedef fftw_plan Plan;
    typedef std::complex<double> Complex;

    static double *alloc_real(std::size_t size) { return fftw_alloc_real(size); }
    static Complex *alloc_complex(std::size_t size) { return reinterpret_cast<Complex *>(fftw_alloc_complex(size)); }
    static void free(void *ptr) { return fftw_free(ptr); }
    static int alignment_of(double *p) { return fftw_alignment_of(p); }

    static int import_wisdom_from_filename(const char *filename) { return fftw_import_wisdom_from_filename(filename); }
    static int export_wisdom_to_filename(const char *filename) { return fftw_export_wisdom_to_filename(filename); }

    static Plan plan_dft_r2c_1d(std::size_t size, double *in, Complex *out, unsigned flags) { return fftw_plan_dft_r2c_1d(size, in, reinterpret_cast<fftw_complex *>(out), flags); }
    static Plan plan_dft_c2r_1d(std::size_t size, Complex *in, double *out, unsigned flags) { return fftw_plan_dft_c2r_1d(size, reinterpret_cast<fftw_complex *>(in), out, flags); }

    static void execute(Plan plan) { fftw_execute(plan); }
    static void execute_dft_r2c(Plan plan, double *in, Complex *out) { fftw_execute_dft_r2c(plan, in, reinterpret_cast<fftw_complex *>(out)); }
    static void execute_dft_c2r(Plan plan, Complex *in, double *out) { fftw_execute_dft_c2r(plan, reinterpret_cast<fftw_complex *>(in), out); }

    static void destroy_plan(Plan plan) { fftw_destroy_plan(plan); }
    static void cleanup() { fftw_cleanup(); }
};

class FftwMissingWisdomException : public jw_util::BaseException {
public:
    FftwMissingWisdomException(const std::string &msg)
        : BaseException(msg)
    {}
};

extern std::mutex fftwMutex;

template <typename ElementType>
class FftwPlanner {
    typedef fftwx_impl<ElementType> fftwx;

public:
    struct IO {
        IO()
#ifndef NDEBUG
            : real(0)
            , complex(0)
#endif
        {}

        IO(ElementType *real, typename fftwx::Complex *complex)
            : real(real)
            , complex(complex)
        {}

        ElementType *real;
        typename fftwx::Complex *complex;

        bool operator==(const IO &other) const {
            return real == other.real && complex == other.complex;
        }
    };

    FftwPlanner() = delete;

    static_assert(sizeof(typename fftwx::Plan) == sizeof(void *), "Unexpected sizeof(fftwx::Plan)");

    template <std::size_t planSize>
    static typename fftwx::Plan getPlanFwd() {
        assert(isInit);
        static constexpr unsigned int planIndex = exactLog2<planSize>();
        static_assert(planIndex < planFwds.size());
        assert(planFwds[planIndex]);
        return planFwds[planIndex];
    }

    template <std::size_t planSize>
    static typename fftwx::Plan getPlanBwd() {
        assert(isInit);
        static constexpr unsigned int planIndex = exactLog2<planSize>();
        static_assert(planIndex < planBwds.size());
        assert(planBwds[planIndex]);
        return planBwds[planIndex];
    }

#ifndef NDEBUG
    static bool &doesThreadHaveOutstandingRequest() {
        static thread_local bool has = false;
        return has;
    }
#endif

    static IO getThreadIO() {
        static thread_local IO io(fftwx::alloc_real(CHUNK_SIZE * 2), fftwx::alloc_complex(CHUNK_SIZE * 2));
        return io;
    }

    static const IO request() {
#ifndef NDEBUG
        assert(doesThreadHaveOutstandingRequest() == false);
        doesThreadHaveOutstandingRequest() = true;
#endif

        return getThreadIO();

        /*
        // TODO: Just allocate these on the stack?
        // TODO: Or even better, just allocate in a static thread_local variable?

        std::unique_lock<std::mutex> lock(fftwMutex);

        IO res;
        if (IOs.empty()) {
            res.real = fftwx::alloc_real(CHUNK_SIZE * 2);
            res.complex = fftwx::alloc_complex(CHUNK_SIZE * 2);
            createdIOs++;
            assert(createdIOs <= std::thread::hardware_concurrency());
        } else {
            res = IOs.back();
            IOs.pop_back();
        }

        return res;
        */
    }

    static void release(const IO io) {
#ifndef NDEBUG
        assert(doesThreadHaveOutstandingRequest() == true);
        doesThreadHaveOutstandingRequest() = false;
#endif

        assert(io.real == getThreadIO().real);
        assert(io.complex == getThreadIO().complex);

        /*
        std::unique_lock<std::mutex> lock(fftwMutex);
        assert(std::find(IOs.cbegin(), IOs.cend(), io) == IOs.cend());
        IOs.emplace_back(io);
        */
    }

    inline static void checkAlignment(ElementType *ptr) {
#ifndef NDEBUG
        static thread_local ElementType *authentic = fftwx::alloc_real(1);
        assert(fftwx::alignment_of(ptr) == fftwx::alignment_of(authentic));
#endif
    }
    inline static void checkAlignment(typename fftwx::Complex *ptr) {
#ifndef NDEBUG
        static thread_local typename fftwx::Complex *authentic = fftwx::alloc_complex(1);
        assert(fftwx::alignment_of(reinterpret_cast<ElementType *>(ptr)) == fftwx::alignment_of(reinterpret_cast<ElementType *>(authentic)));
#endif
    }

    static void init() {
        jw_util::Thread::assert_main_thread();

        if (isInit) {
            return;
        }
        isInit = true;

        static const std::string tn = jw_util::TypeName::get<FftwPlanner<ElementType>>();

        static const std::string filename = app::Options::getInstance().wisdomDir + "/fftw_wisdom_" + jw_util::TypeName::get<ElementType>() + ".bin";
        static const std::string tmpFilename = filename + ".tmp";

        bool importSuccess = fftwx::import_wisdom_from_filename(filename.data());
        if (importSuccess) {
            SPDLOG_INFO("{}::init() - Import from {} was successful", tn, filename);
        } else if (app::Options::getInstance().requireExistingWisdom) {
            throw FftwMissingWisdomException("Missing requried wisdom file at " + filename);
        } else {
            SPDLOG_WARN("{}::init() - Import from {} failed. No worries, we can regenerate it, but it might take a minute or so.", tn, filename);
        }

        IO io = request();

        auto tryCreatePlan = [](unsigned int sizeLog2, auto creatorFunc, const std::string &name) -> typename fftwx::Plan {
            static constexpr unsigned int baseFlags = FFTWX_PLANNING_LEVEL | FFTW_DESTROY_INPUT;

            typename fftwx::Plan res = creatorFunc(1u << sizeLog2, baseFlags | FFTW_WISDOM_ONLY);

            if (res) {
                SPDLOG_INFO("{}::init() - Loaded {}", tn, name);
            } else {
                if (app::Options::getInstance().requireExistingWisdom) {
                    throw FftwMissingWisdomException("Wisdom file at " + filename + " does not include plan for " + name);
                } else {
                    SPDLOG_WARN("{}::init() - Generating {}...", tn, name);
                    res = creatorFunc(1u << sizeLog2, baseFlags);
                    assert(res);

                    if (app::Options::getInstance().writeWisdom) {
                        bool exportSuccess = fftwx::export_wisdom_to_filename(tmpFilename.data());
                        if (exportSuccess) {
                            int failed = std::rename(tmpFilename.data(), filename.data());
                            if (failed) {
                                SPDLOG_ERROR("{}::init() - Rename {} to {} FAILED with return value {} and errno {}", tn, tmpFilename, filename, failed, errno);
                            } else {
                                SPDLOG_INFO("{}::init() - Export to {} was successful", tn, filename);
                            }
                        } else {
                            SPDLOG_ERROR("{}::init() - Export to {} FAILED", tn, tmpFilename);
                        }
                    }
                }
            }

            return res;
        };

        auto fwdCreator = [io](unsigned int size, unsigned int flags) { return fftwx::plan_dft_r2c_1d(size, io.real, io.complex, flags); };
        for (unsigned int i = 0; i < planFwds.size(); i++) {
            planFwds[i] = tryCreatePlan(i, fwdCreator, "forward fft of size 2^" + std::to_string(i));
        }

        auto bwdCreator = [io](unsigned int size, unsigned int flags) { return fftwx::plan_dft_c2r_1d(size, io.complex, io.real, flags); };
        for (unsigned int i = 0; i < planBwds.size(); i++) {
            planBwds[i] = tryCreatePlan(i, bwdCreator, "backward fft of size 2^" + std::to_string(i));
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

        /*
        for (IO io : IOs) {
            fftwx::free(io.real);
            fftwx::free(io.complex);
        }
        IOs.clear();
        */

        fftwx::cleanup();
    }

private:
    inline static bool isInit;
//    inline static std::vector<IO> IOs;
//    inline static unsigned int createdIOs = 0;
    inline static std::array<typename fftwx::Plan, CHUNK_SIZE_LOG2 + 2> planFwds;
    inline static std::array<typename fftwx::Plan, CHUNK_SIZE_LOG2 + 2> planBwds;
};

}

#include "util/testrunner.h"

#include <random>

#include "spdlog/spdlog.h"

#include "series/convseries.h"
#include "series/infcompseries.h"

#include "defs/TEST_KERNEL_MAX_SIZE_LOG2.h"
#include "defs/TEST_TS_MAX_SIZE_LOG2.h"

template <typename ElementType>
class VectorBuilder : public std::vector<ElementType> {
public:
    VectorBuilder &withPush(ElementType value, std::size_t repeat = 1) {
        this->resize(this->size() + repeat, value);
        return *this;
    }
};

template <typename ElementType>
bool testConv(app::AppContext &context, const std::vector<ElementType> kernel, const std::vector<ElementType> ts) {
    series::FiniteCompSeries<ElementType> fin(context, std::vector<ElementType>(kernel));
    auto op = [&ts](ElementType *dst, std::size_t begin, std::size_t end) {
        assert(end - begin == CHUNK_SIZE);

        if (ts.size() < end) {
            if (begin < ts.size()) {
                std::fill(dst + ts.size() - begin, dst + CHUNK_SIZE, NAN);
            } else {
                std::fill_n(dst, CHUNK_SIZE, NAN);
                return;
            }
            end = ts.size();
        }
        std::copy(ts.data() + begin, ts.data() + end, dst);
    };
    series::InfCompSeries<ElementType, decltype(op)> inf(context, op);
    series::ConvSeries<ElementType> conv(context, fin, inf, 0.0, false);

    std::size_t checkChunks = ts.size() / CHUNK_SIZE + 2;

    static thread_local std::vector<ElementType> expected;
    expected.clear();
    expected.resize(checkChunks * CHUNK_SIZE, NAN);
    for (std::size_t i = kernel.size() - 1; i < ts.size(); i++) {
        double sum = 0.0;
        for (std::size_t j = 0; j < kernel.size(); j++) {
            sum += kernel[j] * ts[i - j];
        }
        expected[i] = sum;
    }

    while (true) {
        bool done = true;
        for (std::size_t i = 0; i < checkChunks; i++) {
            done &= conv.getChunk(i)->isDone();
        }

        if (done) {
            break;
        } else {
            std::this_thread::yield();
        }
    }

    static thread_local std::vector<ElementType> actual;
    actual.resize(checkChunks * CHUNK_SIZE);
    for (std::size_t i = 0; i < checkChunks; i++) {
        std::copy_n(conv.getChunk(i)->getData(), CHUNK_SIZE, actual.data() + i * CHUNK_SIZE);
    }

    ElementType allowedError = ts.size() * std::pow(0.04, sizeof(ElementType));
    for (std::size_t i = 0; i < checkChunks * CHUNK_SIZE; i++) {
        ElementType dist = std::fabs(expected[i] - actual[i]);
        bool valid = std::isnan(dist)
                ? std::isnan(expected[i]) == std::isnan(actual[i])
                : dist <= std::max(static_cast<ElementType>(1.0), std::fabs(expected[i]) + std::fabs(actual[i])) * allowedError;

        if (!valid) {
            spdlog::error("FAILURE: At index {}: expected {}, but was {}", i, expected[i], actual[i]);
            return false;
        }
    }

    return true;
}

template <typename ElementType, std::size_t order = 4>
std::vector<ElementType> makeRandomWalk(std::size_t length, double nanChance) {
    static thread_local std::default_random_engine rng;
    std::normal_distribution<ElementType> walkDist(0.0, 1.0);
    std::bernoulli_distribution nanDist(nanChance);

    std::vector<ElementType> vec;
    vec.resize(length);

    ElementType dvs[order];
    for (std::size_t j = 0; j < order; j++) {
        dvs[j] = walkDist(rng);
    }

    for (std::size_t i = 0; i < length; i++) {
        dvs[order - 1] = walkDist(rng);
        for (std::size_t j = 0; j < order - 1; j++) {
            dvs[j] = dvs[j] * 0.99 + dvs[j + 1] * 0.1;
        }
        vec[i] = nanDist(rng) ? NAN : dvs[0];
    }

    return std::move(vec);
}

template <typename ElementType>
void test(app::AppContext &context) {
    static thread_local std::default_random_engine rng;
    std::normal_distribution<double> additionalDist(0.0, 0.1);

    for (unsigned int i = 0; i < TEST_KERNEL_MAX_SIZE_LOG2; i++) {
        for (unsigned int j = i - 1; j < TEST_TS_MAX_SIZE_LOG2; j++) {
            double nanChance = std::max(0.0, std::min(4.0 / (1 << j), 0.5));
            std::size_t kernelSize = std::max(1.0, (1 << i) * std::exp(additionalDist(rng)));
            std::size_t tsSize = std::max(1.0, (1 << j) * std::exp(additionalDist(rng)));
            bool res = testConv<ElementType>(
                        context,
                        makeRandomWalk<ElementType>(kernelSize, 0.0),
                        makeRandomWalk<ElementType>(tsSize, nanChance)
                        );
            spdlog::info("{} test with kernel.sizeLog2={} and ts.sizeLog2={}", res ? "PASSED" : "FAILED", i, j);
            if (!res) {
                std::exit(1);
            }
        }
    }
}

static int _0 = util::TestRunner::getInstance().registerTest(test<float>);
static int _1 = util::TestRunner::getInstance().registerTest(test<double>);

#include "program/resolver.h"
#include "series/finitecompseries.h"

template <typename RealType>
auto windowRect(app::AppContext &context, RealType width) {
    std::vector<RealType> data;
    data.resize(width, 1.0 / width);
    return new series::FiniteCompSeries<RealType>(context, std::move(data));
}

template <typename RealType>
auto windowSimple(app::AppContext &context, RealType scale_0) {
    std::size_t width = std::sqrt(-std::log(1e-9)) * scale_0 + 1.0;

    std::vector<RealType> data;
    data.resize(width);

    double sum = 0.0;
    for (std::size_t i = 0; i < width; i++) {
        double t = i / scale_0;
        t = std::exp(-t * t);
        data[i] = t;
        sum += t;
    }
    for (std::size_t i = 0; i < width; i++) {
        data[i] /= sum;
    }

    return new series::FiniteCompSeries<RealType>(context, std::move(data));
}

template <typename RealType>
auto windowSmooth(app::AppContext &context, RealType scale_0, RealType scale_1_mult = 2.0) {
    RealType scale_1 = scale_0 * scale_1_mult;
    std::size_t width = std::sqrt(-std::log(1e-9)) * std::max(scale_0, scale_1) + 1.0;

    std::vector<RealType> data;
    data.resize(width);

    double sum = 0.0;
    for (std::size_t i = 0; i < width; i++) {
        double head = i / scale_0;
        head = std::exp(-head * head);
        double tail = i / scale_1;
        tail = std::exp(-tail * tail);
        double t = head - tail;
        data[i] = t;
        sum += t;
    }
    for (std::size_t i = 0; i < width; i++) {
        data[i] /= sum;
    }

    return new series::FiniteCompSeries<RealType>(context, std::move(data));
}

template <typename RealType>
auto windowDelta(app::AppContext &context, RealType scale_0, RealType scale_1_mult = 2.0) {
    RealType scale_1 = scale_0 * scale_1_mult;
    std::size_t width = std::sqrt(-std::log(1e-9)) * std::max(scale_0, scale_1) + 1.0;

    std::vector<RealType> data;
    data.resize(width);
    static thread_local std::vector<RealType> tmp;
    tmp.resize(width);

    double headSum = 0.0;
    double tailSum = 0.0;
    for (std::size_t i = 0; i < width; i++) {
        double head = i / scale_0;
        head = std::exp(-head * head);
        data[i] = head;
        headSum += head;

        double tail = i / scale_1;
        tail = std::exp(-tail * tail);
        tmp[i] = tail;
        tailSum += tail;
    }
    for (std::size_t i = 0; i < width; i++) {
        data[i] = data[i] / headSum - tmp[i] / tailSum;
    }

    return new series::FiniteCompSeries<RealType>(context, std::move(data));
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("window_rect", [&context](float scale_0){return windowRect(context, scale_0);});
    resolver.decl("window_rect", [&context](double scale_0){return windowRect(context, scale_0);});

    resolver.decl("window_simple", [&context](float scale_0){return windowSimple(context, scale_0);});
    resolver.decl("window_simple", [&context](double scale_0){return windowSimple(context, scale_0);});

    resolver.decl("window_smooth", [&context](float scale_0, float scale_1){return windowSmooth(context, scale_0, scale_1);});
    resolver.decl("window_smooth", [&context](double scale_0, double scale_1){return windowSmooth(context, scale_0, scale_1);});

    resolver.decl("window_delta", [&context](float scale_0, float scale_1){return windowDelta(context, scale_0, scale_1);});
    resolver.decl("window_delta", [&context](double scale_0, double scale_1){return windowDelta(context, scale_0, scale_1);});
});

#include "resolver.h"

#include "series/inputseries.h"
#include "series/staticseries.h"
#include "series/parallelopseries.h"
#include "series/convseries.h"

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"

namespace {

template <typename RealType>
auto funcAddParallel(app::AppContext &context, series::DataSeries<RealType> *a, series::DataSeries<RealType> *b) {
    auto op = [](RealType a, RealType b) {return a + b;};
    return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>, series::DataSeries<RealType>>(context, op, *a, *b);
}

template <typename RealType>
auto funcAddEach(app::AppContext &context, series::DataSeries<RealType> *a, RealType b) {
    auto op = [b](RealType a) {return a + b;};
    return new series::ParallelOpSeries<RealType, decltype(op), series::DataSeries<RealType>>(context, op, *a);
}

template <typename RealType>
auto windowRect(app::AppContext &context, RealType width) {
    auto op = [width](double *dst, std::size_t begin, std::size_t end) {
        std::fill_n(dst, end - begin, 1.0 / width);
    };

    return new series::StaticSeries<double, decltype(op)>(context, op, width);
}

template <typename RealType>
auto windowSimple(app::AppContext &context, RealType scale_0) {
    auto op = [scale_0](double *dst, std::size_t begin, std::size_t end) {
        double sum = 0.0;
        for (std::size_t i = begin; i < end; i++) {
            double t = i / scale_0;
            t = std::exp(-t * t);
            dst[i - begin] = t;
            sum += t;
        }
        for (std::size_t i = begin; i < end; i++) {
            dst[i - begin] /= sum;
        }
    };

    std::size_t width = std::sqrt(-std::log(1e-9)) * scale_0 + 1.0;
    return new series::StaticSeries<double, decltype(op)>(context, op, width);
}

template <typename RealType>
auto convolve(app::AppContext &context, series::DataSeries<RealType> *kernel, series::DataSeries<RealType> *ts) {
    return new series::ConvSeries<RealType>(context, *kernel, *ts);
}

}

namespace program {

Resolver::Resolver(app::AppContext &context)
    : context(context)
{
    decl("input", [&context](const std::string &name){return new series::InputSeries<INPUT_SERIES_ELEMENT_TYPE>(context, name);});

    decl("cast_float", [](UncastNumber num){return static_cast<float>(num.value);});
    decl("cast_double", [](UncastNumber num){return static_cast<double>(num.value);});

    decl("add", [](float a, float b){return a + b;});
    decl("add", [](float a, double b){return a + b;});
    decl("add", [](double a, float b){return a + b;});
    decl("add", [](double a, double b){return a + b;});
    decl("add", [&context](series::DataSeries<float> *a, float b){return funcAddEach(context, a, b);});
    decl("add", [&context](series::DataSeries<double> *a, double b){return funcAddEach(context, a, b);});
    decl("add", [&context](float a, series::DataSeries<float> *b){return funcAddEach(context, b, a);});
    decl("add", [&context](double a, series::DataSeries<double> *b){return funcAddEach(context, b, a);});
    decl("add", [&context](series::DataSeries<float> *a, series::DataSeries<float> *b){return funcAddParallel(context, a, b);});
    decl("add", [&context](series::DataSeries<double> *a, series::DataSeries<double> *b){return funcAddParallel(context, a, b);});

    decl("window_rect", [&context](float scale_0){return windowRect(context, scale_0);});
    decl("window_rect", [&context](double scale_0){return windowRect(context, scale_0);});

    decl("window_simple", [&context](float scale_0){return windowSimple(context, scale_0);});
    decl("window_simple", [&context](double scale_0){return windowSimple(context, scale_0);});

    decl("conv", [&context](series::DataSeries<float> *kernel, series::DataSeries<float> *ts){return convolve(context, kernel, ts);});
    decl("conv", [&context](series::DataSeries<double> *kernel, series::DataSeries<double> *ts){return convolve(context, kernel, ts);});
}

ProgObj Resolver::call(const std::string &name, const std::vector<ProgObj> &args) {
    auto foundValue = calls.emplace(Call(name, args), ProgObj());
    if (foundValue.second) {
        auto foundImpl = declarations.find(Decl(name, Decl::calcArgTypeComb(args)));
        if (foundImpl == declarations.cend()) {
            std::string msg = "Unable to resolve " + name + "(";

            for (const ProgObj &obj : args) {
                msg += progObjTypeNames[obj.index()];
                msg += ", ";
            }

            msg.pop_back();
            msg.back() = ')';

            throw UnresolvedCallException(msg);
        }

        foundValue.first->second = foundImpl->second->invoke(args);
    }
    return foundValue.first->second;
}

}

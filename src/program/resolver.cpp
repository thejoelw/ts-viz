#include "resolver.h"

#include "series/inputseries.h"
#include "series/finitecompseries.h"
#include "series/infcompseries.h"
#include "series/parallelopseries.h"
#include "series/scannedseries.h"
#include "series/convseries.h"
#include "render/dataseriesrenderer.h"

#include "defs/INPUT_SERIES_ELEMENT_TYPE.h"

namespace {

template <typename RealType> struct FuncSgn { RealType operator()(RealType a) const { return (RealType(0) < a) - (a < RealType(0)); } };
template <typename RealType> struct FuncAbs { RealType operator()(RealType a) const { return std::abs(a); } };
template <typename RealType> struct FuncSquare { RealType operator()(RealType a) const { return a * a; } };
template <typename RealType> struct FuncSqrt { RealType operator()(RealType a) const { return std::sqrt(a); } };
template <typename RealType> struct FuncExp { RealType operator()(RealType a) const { return std::exp(a); } };
template <typename RealType> struct FuncLog { RealType operator()(RealType a) const { return std::log(a); } };

template <typename RealType> struct FuncMod { RealType operator()(RealType a, RealType b) const { return std::fmod(a, b); } };
template <typename RealType> struct FuncMinimum { RealType operator()(RealType a, RealType b) const { return std::min(a, b); } };
template <typename RealType> struct FuncMaximum { RealType operator()(RealType a, RealType b) const { return std::max(a, b); } };
template <typename RealType> struct FuncShrink { RealType operator()(RealType a, RealType b) const { return a > b ? a - b : a < -b ? a + b : 0; } };

template <template <typename> typename Operator>
void declUnaryOp(program::Resolver *resolver, app::AppContext &context, const char *funcName) {
    resolver->decl(funcName, Operator<float>());
    resolver->decl(funcName, Operator<double>());
    resolver->decl(funcName, [&context](series::DataSeries<float> *a){
        return new series::ParallelOpSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(), *a);
    });
    resolver->decl(funcName, [&context](series::DataSeries<double> *a){
        return new series::ParallelOpSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(), *a);
    });
}

template <template <typename> typename Operator>
void declBinaryOp(program::Resolver *resolver, app::AppContext &context, const char *funcName) {
    resolver->decl(funcName, [](float a, float b){return Operator<float>()(a, b);});
    resolver->decl(funcName, [](float a, double b){return Operator<double>()(a, b);});
    resolver->decl(funcName, [](double a, float b){return Operator<double>()(a, b);});
    resolver->decl(funcName, [](double a, double b){return Operator<double>()(a, b);});
    resolver->decl(funcName, [&context](series::DataSeries<float> *a, float b){
        auto op = [b](float a) {return Operator<float>()(a, b);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<float>>(context, op, *a);
    });
    resolver->decl(funcName, [&context](series::DataSeries<double> *a, double b){
        auto op = [b](double a) {return Operator<double>()(a, b);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<double>>(context, op, *a);
    });
    resolver->decl(funcName, [&context](float a, series::DataSeries<float> *b){
        auto op = [a](float b) {return Operator<float>()(a, b);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<float>>(context, op, *b);
    });
    resolver->decl(funcName, [&context](double a, series::DataSeries<double> *b){
        auto op = [a](double b) {return Operator<double>()(a, b);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<double>>(context, op, *b);
    });
    resolver->decl(funcName, [&context](series::DataSeries<float> *a, series::DataSeries<float> *b){
        return new series::ParallelOpSeries<float, Operator<float>, series::DataSeries<float>, series::DataSeries<float>>(context, Operator<float>(), *a, *b);
    });
    resolver->decl(funcName, [&context](series::DataSeries<double> *a, series::DataSeries<double> *b){
        return new series::ParallelOpSeries<double, Operator<double>, series::DataSeries<double>, series::DataSeries<double>>(context, Operator<double>(), *a, *b);
    });
}

template <template <typename> typename Operator>
void declScanOp(program::Resolver *resolver, app::AppContext &context, const char *funcName, double initialValue) {
    resolver->decl(funcName, [&context, initialValue](series::DataSeries<float> *a){
        return new series::ScannedSeries<float, Operator<float>, series::DataSeries<float>>(context, Operator<float>(), initialValue, *a);
    });
    resolver->decl(funcName, [&context, initialValue](series::DataSeries<double> *a){
        return new series::ScannedSeries<double, Operator<double>, series::DataSeries<double>>(context, Operator<double>(), initialValue, *a);
    });
}

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

template <typename RealType>
auto sequence(app::AppContext &context, RealType scale) {
    auto op = [scale](RealType *dst, std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; i++) {
            dst[i - begin] = i * scale;
        }
    };

    return new series::InfCompSeries<RealType, decltype(op)>(context, op);
}

}

namespace program {

Resolver::Resolver(app::AppContext &context)
    : context(context)
{
    decl("input", [&context](const std::string &name){return new series::InputSeries<INPUT_SERIES_ELEMENT_TYPE>(context, name);});

    decl("cast_float", [](UncastNumber num){return static_cast<float>(num.value);});
    decl("cast_float", [](float num) {return static_cast<float>(num);});
    decl("cast_float", [](double num) {return static_cast<float>(num);});
    decl("cast_float", [](series::DataSeries<float> *a){
        return a;
    });
    decl("cast_float", [&context](series::DataSeries<double> *a){
        auto op = [](double a) {return static_cast<float>(a);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<double>>(context, op, *a);
    });

    decl("cast_double", [](UncastNumber num){return static_cast<double>(num.value);});
    decl("cast_double", [](float num) {return static_cast<double>(num);});
    decl("cast_double", [](double num) {return static_cast<double>(num);});
    decl("cast_double", [&context](series::DataSeries<float> *a){
        auto op = [](float a) {return static_cast<double>(a);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<float>>(context, op, *a);
    });
    decl("cast_double", [](series::DataSeries<double> *a){
        return a;
    });

    declUnaryOp<FuncSgn>(this, context, "sgn");
    declUnaryOp<FuncAbs>(this, context, "abs");
    declUnaryOp<FuncSquare>(this, context, "square");
    declUnaryOp<FuncSqrt>(this, context, "sqrt");
    declUnaryOp<FuncExp>(this, context, "exp");
    declUnaryOp<FuncLog>(this, context, "log");

    declBinaryOp<std::plus>(this, context, "add");
    declBinaryOp<std::minus>(this, context, "sub");
    declBinaryOp<std::multiplies>(this, context, "mul");
    declBinaryOp<std::divides>(this, context, "div");
    declBinaryOp<FuncMod>(this, context, "mod");
    declBinaryOp<std::less>(this, context, "lt");
    declBinaryOp<std::greater>(this, context, "gt");
    declBinaryOp<FuncMinimum>(this, context, "min");
    declBinaryOp<FuncMaximum>(this, context, "max");
    declBinaryOp<FuncShrink>(this, context, "shrink");

    declScanOp<std::plus>(this, context, "cumsum", 0.0);
    declScanOp<std::multiplies>(this, context, "cumprod", 1.0);

    decl("window_rect", [&context](float scale_0){return windowRect(context, scale_0);});
    decl("window_rect", [&context](double scale_0){return windowRect(context, scale_0);});

    decl("window_simple", [&context](float scale_0){return windowSimple(context, scale_0);});
    decl("window_simple", [&context](double scale_0){return windowSimple(context, scale_0);});

    decl("window_smooth", [&context](float scale_0, float scale_1){return windowSmooth(context, scale_0, scale_1);});
    decl("window_smooth", [&context](double scale_0, double scale_1){return windowSmooth(context, scale_0, scale_1);});

    decl("window_delta", [&context](float scale_0, float scale_1){return windowDelta(context, scale_0, scale_1);});
    decl("window_delta", [&context](double scale_0, double scale_1){return windowDelta(context, scale_0, scale_1);});

    decl("conv", [&context](series::FiniteCompSeries<float> *kernel, series::DataSeries<float> *ts){return new series::ConvSeries<float>(context, *kernel, *ts);});
    decl("conv", [&context](series::FiniteCompSeries<double> *kernel, series::DataSeries<double> *ts){return new series::ConvSeries<double>(context, *kernel, *ts);});

    decl("seq", [&context](float scale){return sequence(context, scale);});
    decl("seq", [&context](double scale){return sequence(context, scale);});

    decl("plot", [&context](series::DataSeries<float> *s, const std::string &name, UncastNumber r, UncastNumber g, UncastNumber b, UncastNumber a){
        render::DataSeriesRenderer<float> *res = new render::DataSeriesRenderer<float>(context, s, name);
        res->getDrawStyle().color[0] = r.value;
        res->getDrawStyle().color[1] = g.value;
        res->getDrawStyle().color[2] = b.value;
        res->getDrawStyle().color[3] = a.value;
        return res;
    });

    decl("plot", [&context](series::DataSeries<double> *s, const std::string &name, UncastNumber r, UncastNumber g, UncastNumber b, UncastNumber a){
        render::DataSeriesRenderer<double> *res = new render::DataSeriesRenderer<double>(context, s, name);
        res->getDrawStyle().color[0] = r.value;
        res->getDrawStyle().color[1] = g.value;
        res->getDrawStyle().color[2] = b.value;
        res->getDrawStyle().color[3] = a.value;
        return res;
    });
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

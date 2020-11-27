#include "program/resolver.h"
#include "series/type/compseries.h"

template <typename RealType, typename OperatorType>
auto makeInf(app::AppContext &context, OperatorType op) {
    return new series::CompSeries<RealType, decltype(op)>(context, op);
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("const", [&context](float value){return makeInf<float>(context, [value](std::size_t i) {(void) i; return value;});});
    resolver.decl("const", [&context](double value){return makeInf<double>(context, [value](std::size_t i) {(void) i; return value;});});

    resolver.decl("seq", [&context](float scale){return makeInf<float>(context, [scale](std::size_t i) {return i * scale;});});
    resolver.decl("seq", [&context](double scale){return makeInf<double>(context, [scale](std::size_t i) {return i * scale;});});

    resolver.decl("sin", [&context](float wavelength){return makeInf<float>(context, [wavelength](std::size_t i) {return std::sin(i / wavelength);});});
    resolver.decl("sin", [&context](double wavelength){return makeInf<double>(context, [wavelength](std::size_t i) {return std::sin(i / wavelength);});});

    resolver.decl("cos", [&context](float wavelength){return makeInf<float>(context, [wavelength](std::size_t i) {return std::cos(i / wavelength);});});
    resolver.decl("cos", [&context](double wavelength){return makeInf<double>(context, [wavelength](std::size_t i) {return std::cos(i / wavelength);});});

    resolver.decl("gaussian", [&context](float wavelength){return makeInf<float>(context, [wavelength](std::size_t i) {float x = i / wavelength; return std::exp(-x * x);});});
    resolver.decl("gaussian", [&context](double wavelength){return makeInf<double>(context, [wavelength](std::size_t i) {double x = i / wavelength; return std::exp(-x * x);});});
});

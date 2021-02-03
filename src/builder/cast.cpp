#include "program/resolver.h"
#include "series/type/parallelopseries.h"

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("cast_float", [](program::UncastNumber num){return static_cast<float>(num.value);});
    resolver.decl("cast_float", [](float num) {return static_cast<float>(num);});
    resolver.decl("cast_float", [](double num) {return static_cast<float>(num);});
    resolver.decl("cast_float", [](std::int64_t num) {return static_cast<float>(num);});
    resolver.decl("cast_float", [](series::DataSeries<float> *a){
        return a;
    });
    resolver.decl("cast_float", [&context](series::DataSeries<double> *a){
        auto op = [](double a) {return static_cast<float>(a);};
        return new series::ParallelOpSeries<float, decltype(op), series::DataSeries<double>>(context, op, *a);
    });

    resolver.decl("cast_double", [](program::UncastNumber num){return static_cast<double>(num.value);});
    resolver.decl("cast_double", [](float num) {return static_cast<double>(num);});
    resolver.decl("cast_double", [](double num) {return static_cast<double>(num);});
    resolver.decl("cast_double", [](std::int64_t num) {return static_cast<double>(num);});
    resolver.decl("cast_double", [&context](series::DataSeries<float> *a){
        auto op = [](float a) {return static_cast<double>(a);};
        return new series::ParallelOpSeries<double, decltype(op), series::DataSeries<float>>(context, op, *a);
    });
    resolver.decl("cast_double", [](series::DataSeries<double> *a){
        return a;
    });

    resolver.decl("cast_int64", [](program::UncastNumber num){return static_cast<std::int64_t>(num.value);});
    resolver.decl("cast_int64", [](float num){return static_cast<std::int64_t>(num);});
    resolver.decl("cast_int64", [](double num){return static_cast<std::int64_t>(num);});
    resolver.decl("cast_int64", [](std::int64_t num) {return static_cast<std::int64_t>(num);});
});

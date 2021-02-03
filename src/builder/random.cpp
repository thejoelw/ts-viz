#include "program/resolver.h"
#include "series/type/compseries.h"

#include <random>

/*
template <template DistGenerator, template... ArgTypes>
void declRandomOp(app::AppContext &context, program::Resolver &resolver, const char *funcName) {
    resolver.decl(funcName, [&context](ArgTypes... args){return makeInf<float>(context, [rng](std::size_t i) {float x = i / wavelength; return std::exp(-x * x);});});
}

template <template <typename> typename DistGenerator>
auto makeRandom(app::AppContext &context, OperatorType dist) {
    typedef typename OperatorType::result_type RealType;

    auto op = [dist]
    return new series::CompSeries<RealType, decltype(op)>(context, op);

    std::default_random_engine generator;
    std::normal_distribution<double> distribution(5.0,2.0);

    int p[10]={};

    for (int i=0; i<nrolls; ++i) {
      double number = distribution(generator);
      if ((number>=0.0)&&(number<10.0)) ++p[int(number)];
    }


}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    resolver.decl("rand_normal", [&context](float mean, float std) {return makeRandom(context, std::normal_distribution<float>(mean, std));});
    resolver.decl("rand_normal", [&context](double mean, double std) {return makeRandom(context, std::normal_distribution<double>(mean, std));});
});
*/

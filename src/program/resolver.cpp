#include "resolver.h"

#include "series/staticseries.h"

namespace program {

Resolver::Resolver(app::AppContext &context)
    : context(context)
{
    decl("cast_float", [](UncastNumber num){return static_cast<float>(num.value);});
    decl("cast_double", [](UncastNumber num){return static_cast<double>(num.value);});

    decl("add", [](float a, float b){return a + b;});
    decl("add", [](float a, double b){return a + b;});
    decl("add", [](double a, float b){return a + b;});
    decl("add", [](double a, double b){return a + b;});

    decl("window_simple_double", [&context](double scale_0){
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

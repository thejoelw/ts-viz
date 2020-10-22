#include "resolver.h"

namespace program {

Resolver::Resolver(app::AppContext &context)
    : context(context)
{
    for (std::function<void (app::AppContext &, Resolver &)> &func : builders) {
        func(context, *this);
    }
}

ProgObj Resolver::call(const std::string &name, const std::vector<ProgObj> &args) {
    auto foundValue = calls.emplace(Call(name, args), ProgObj());
    if (foundValue.second) {
        auto foundImpl = declarations.find(Decl(name, Decl::calcArgTypeComb(args)));
        if (foundImpl == declarations.cend()) {
            calls.erase(foundValue.first);

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

int Resolver::registerBuilder(std::function<void (app::AppContext &, Resolver &)> func) {
    builders.push_back(func);
    return 0;
}

std::vector<std::function<void(app::AppContext &, Resolver &)>> Resolver::builders;

}

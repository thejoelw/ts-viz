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
        try {
            foundValue.first->second = execDecl(name, args);
        } catch (const UnresolvedCallException &ex) {
            calls.erase(foundValue.first);
            throw ex;
        }
    }
    return foundValue.first->second;
}

template <typename ItemType>
static ProgObjArray<ItemType> extractArray(const std::vector<ProgObj> &args) {
    std::vector<ItemType> vec;
    for (const ProgObj &obj : args) {
        vec.push_back(std::get<ItemType>(obj));
    }
    return ProgObjArray<ItemType>(std::move(vec));
}

ProgObj Resolver::execDecl(const std::string &name, const std::vector<ProgObj> &args) {
    if (name == "arr" && args.size() > 0) {
        try { return extractArray<float>(args); } catch (const std::bad_variant_access& ex) {}
        try { return extractArray<double>(args); } catch (const std::bad_variant_access& ex) {}
        try { return extractArray<series::DataSeries<float> *>(args); } catch (const std::bad_variant_access& ex) {}
        try { return extractArray<series::DataSeries<double> *>(args); } catch (const std::bad_variant_access& ex) {}
    }

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

    return foundImpl->second->invoke(args);
}

int Resolver::registerBuilder(std::function<void (app::AppContext &, Resolver &)> func) {
    builders.push_back(func);
    return 0;
}

std::vector<std::function<void(app::AppContext &, Resolver &)>> Resolver::builders;

}

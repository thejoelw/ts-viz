#include "resolver.h"

#include "spdlog/spdlog.h"

#include "app/appcontext.h"
#include "series/dataseries.h"

namespace {

#if ENABLE_CHUNK_NAMES
template <class Type>
auto setName(Type *obj, const std::string &name, int) -> decltype(obj->setName(name), void()) {
    obj->setName(name);
}
template <class Type>
auto setName(Type &obj, const std::string &name, int) -> decltype(obj.setName(name), void()) {
    obj.setName(name);
}
template <class Type>
void setName(Type &, const std::string &, long) {}
#endif

}

namespace program {

Resolver::Resolver(app::AppContext &context)
    : context(context)
{
    for (std::function<void (app::AppContext &, Resolver &)> &func : getBuilders()) {
        func(context, *this);
    }

    spdlog::info("Registered {} declarations", declarations.size());
}

ProgObj Resolver::call(const std::string &name, const std::vector<ProgObj> &args) {
    auto foundValue = calls.emplace(Call(name, args), ProgObj());
    if (foundValue.second) {
        try {
            foundValue.first->second = execDecl(name, args);
        } catch (const std::exception &ex) {
            calls.erase(foundValue.first);
            throw;
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

    ProgObj res = foundImpl->second->invoke(args);
#if ENABLE_CHUNK_NAMES
    std::visit([name](auto &arg){setName(arg, name, 0);}, res);
#endif
    return res;
}

int Resolver::registerBuilder(std::function<void (app::AppContext &, Resolver &)> func) {
    getBuilders().push_back(func);
    return 0;
}

std::vector<std::function<void(app::AppContext &, Resolver &)>> &Resolver::getBuilders() {
    static std::vector<std::function<void(app::AppContext &, Resolver &)>> builders;
    return builders;
}

}

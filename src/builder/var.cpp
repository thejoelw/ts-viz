#include "program/resolver.h"

/*
#include "program/variable.h"
#include "program/variablemanager.h"

template <std::size_t I = 0>
V parse(const json& j)
{
    if constexpr (I < std::variant_size_v<V>)
    {
        auto result = j.get<std::optional<std::variant_alternative_t<I, V>>>();

        return result ? std::move(*result) : parse<I + 1>(j);
    }
    throw ParseError("Can't parse");
}

template <typename Operator, std::size_t typeIndex = 0>
void decl(program::Resolver &resolver, const char *funcName, Operator op) {
    if constexpr (typeIndex < std::variant_size_v<program::ProgObj>) {
        typedef std::variant_alternative_t<typeIndex, program::ProgObj> ArgType;

        resolver.decl(funcName, [op](const std::string &key, ArgType value) {
            return op(key, value)
        });

        decl<Operator, typeIndex + 1>(resolver, funcName, op);
    }
}

static int _ = program::Resolver::registerBuilder([](app::AppContext &context, program::Resolver &resolver) {
    auto setter = [](const std::string &key, auto value) { return new program::Variable(key, value); };
    decl(resolver, "var_set", setter);

    auto getter = [&context](const std::string &key, auto value) { return context.get<program::VariableManager>().loo };
    decl(resolver, "var_get", getter);
});
*/

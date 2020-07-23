#ifndef JWUTIL_REGISTRY_H
#define JWUTIL_REGISTRY_H

#include <unordered_map>
#include <spdlog/fmt/fmt.h>

#include "jw_util/baseexception.h"
#include "jw_util/preparedcontext.h"

namespace jw_util {

template <typename ElementType>
class Registry {
public:
    class AccessException : public BaseException {
    public:
        AccessException(const std::string &key)
            : BaseException(fmt::format("Could not find key \"{}\" in registry", key))
        {}
    };

    template <typename... ContextArgs>
    Registry(Context<ContextArgs...> context) {
        YAML::const_iterator i = context.template get<Config::Node>().begin();
        while (i != context.template get<Config::Node>().end()) {
            auto res = map.emplace(i->first, context.extend(i->second));
            assert(res.second);
            i++;
        }
    }

    ElementType &get(const std::string &key) {
        typename std::unordered_map<std::string, ElementType>::const_iterator found = map.find(key);
        if (found != map.cend()) {
            return map;
        } else {
            throw AccessException(key);
        }
    }

    std::unordered_map<std::string, ElementType> &getMap() {
        return map;
    }

private:
    std::unordered_map<std::string, ElementType> map;
};

}

#endif // JWUTIL_REGISTRY_H

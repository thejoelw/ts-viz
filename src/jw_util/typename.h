#ifndef JWUTIL_TYPENAME_H
#define JWUTIL_TYPENAME_H

#include <string>
#include <typeindex>
#include <cxxabi.h>

namespace jw_util {

class TypeName {
public:
    template <typename Type>
    static std::string get() {
        static thread_local std::string str = make(typeid(Type));
        return str;
    }

private:
    static std::string make(std::type_index typeId) {
        int status = 0;
        std::size_t length;
        char *realname = abi::__cxa_demangle(typeId.name(), 0, &length, &status);
        assert(status == 0);

        std::string str(realname, length - 1);

        std::free(realname);

        return str;
    }
};

}

#endif // JWUTIL_TYPENAME_H

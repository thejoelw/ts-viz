#pragma once

#include <typeindex>
#include <variant>

#include "program/progobj.h"
#include "jw_util/hash.h"
#include "util/hashforwarder.h"

namespace {

template <typename> struct tag {};

template <typename T, typename V>
struct get_variant_index;

template <typename T, typename... Ts>
struct get_variant_index<T, std::variant<Ts...>> : std::integral_constant<std::size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {};

}

namespace app { class AppContext; }

namespace program {

class Resolver {
public:
    class UnresolvedCallException : public jw_util::BaseException {
        friend class Resolver;

    private:
        UnresolvedCallException(const std::string &msg)
            : BaseException(msg)
        {}
    };

    Resolver(app::AppContext &context);

    ProgObj call(const std::string &name, const std::vector<ProgObj> &args);

private:
    template <typename ReturnType, typename... ArgTypes>
    void decl(const std::string &name, ReturnType (*cb)(ArgTypes...)) {
        std::unique_ptr<Invokable> ptr = std::make_unique<FuncPtrWrapper<ReturnType, ArgTypes...>>(cb);
        declarations.emplace(Decl(name, Decl::calcArgTypeComb(std::vector<ProgObj>{ArgTypes{}...})), std::move(ptr));
    }

    class Invokable {
    public:
        virtual ~Invokable() {}
        virtual ProgObj invoke(const std::vector<ProgObj> &args) = 0;
    };

    template <typename ReturnType, typename... ArgTypes>
    class FuncPtrWrapper : public Invokable {
    public:
        FuncPtrWrapper(ReturnType (*ptr)(ArgTypes...))
            : ptr(ptr)
        {}

        ProgObj invoke(const std::vector<ProgObj> &args) {
            return invokeSeq(args, std::make_index_sequence<sizeof...(ArgTypes)>());
        }

    private:
        ReturnType (*ptr)(ArgTypes...);

        template <std::size_t... Indices>
        ProgObj invokeSeq(const std::vector<ProgObj>& args, std::index_sequence<Indices...>) {
            return ptr(std::get<ArgTypes>(args[Indices])...);
        }
    };

    struct Decl {
        Decl(const std::string &name, std::uint64_t argTypeComb)
            : name(name)
            , argTypeComb(argTypeComb)
        {}

        static std::uint64_t calcArgTypeComb(const std::vector<ProgObj> &args) {
            std::uint64_t res = 0;
            for (const ProgObj &obj : args) {
                std::uint64_t prev = res;

                res *= std::variant_size_v<ProgObj>;
                res += obj.index();

                // Make sure we don't overflow
                assert(res >= prev);
            }
            return res;
        }

        std::string name;
        std::uint64_t argTypeComb;

        std::size_t getHash() const {
            std::size_t hash = 0;
            hash = jw_util::Hash<std::size_t>::combine(hash, std::hash<std::string>{}(name));
            hash = jw_util::Hash<std::size_t>::combine(hash, std::hash<std::uint64_t>{}(argTypeComb));
            return hash;
        }
        bool operator==(const Decl &other) const {
            return name == other.name && argTypeComb == other.argTypeComb;
        }
    };

    struct Call {
        Call(const std::string &name, const std::vector<ProgObj> &args)
            : name(name)
            , args(args)
        {}

        std::string name;
        std::vector<ProgObj> args;

        std::size_t getHash() const {
            std::size_t hash = std::hash<std::string>{}(name);
            for (const ProgObj &arg : args) {
                hash = jw_util::Hash<std::size_t>::combine(hash, std::hash<ProgObj>{}(arg));
            }
            return hash;
        }
        bool operator==(const Call &other) const {
            if (name != other.name) {
                return false;
            }
            if (args.size() != other.args.size()) {
                return false;
            }
            for (std::size_t i = 0; i < args.size(); i++) {
                if (!(args[i] == other.args[i])) {
                    return false;
                }
            }
            return true;
        }
    };

    app::AppContext &context;

    std::unordered_map<Decl, std::unique_ptr<Invokable>, util::HashForwarder<Decl>> declarations;
    std::unordered_map<Call, ProgObj, util::HashForwarder<Call>> calls;

    template <typename ReturnType, typename... ArgTypes>
    static ProgObj invokeStub(void (*cbPtr)(), const std::vector<ProgObj> &args) {
        ReturnType (*cb)(ArgTypes...) = static_cast<ReturnType (*)(ArgTypes...)>(cbPtr);
        return invokeSeq(cb, args, std::make_index_sequence<sizeof...(ArgTypes)>());
    }
    template <typename ReturnType, typename... ArgTypes, std::size_t... Indices>
    static ProgObj invokeSeq(ReturnType (*cb)(ArgTypes...), const std::vector<ProgObj>& args, std::index_sequence<Indices...>) {
        return cb(std::get<ArgTypes>(args[Indices])...);
    }
};

}

#ifndef JWUTIL_METHODCALLBACK_H
#define JWUTIL_METHODCALLBACK_H

#include <assert.h>
#include <utility>

namespace jw_util
{

template <typename... ArgTypes>
class MethodCallback
{
public:
    MethodCallback()
        : stub_ptr(0)
    {}

    static MethodCallback<ArgTypes...> create_dummy()
    {
        return MethodCallback(static_cast<void*>(0), &dummy_stub);
    }

    template <void (*Function)(ArgTypes...)>
    static MethodCallback<ArgTypes...> create()
    {
        return MethodCallback(static_cast<void*>(0), &function_stub<Function>);
    }

    template <typename ClassType, void (ClassType::*Method)(ArgTypes...)>
    static MethodCallback<ArgTypes...> create(ClassType *inst)
    {
        return MethodCallback(static_cast<void*>(inst), &method_stub<ClassType, Method>);
    }

    bool is_valid() const
    {
        return stub_ptr;
    }

    void call(ArgTypes... args) const
    {
        assert(stub_ptr);
        (*stub_ptr)(inst_ptr, std::forward<ArgTypes>(args)...);
    }

    template <typename ClassType>
    ClassType *get_inst() const
    {
        return static_cast<ClassType *>(inst_ptr);
    }

    bool is_same_method(const MethodCallback<ArgTypes...> &other) const
    {
        return stub_ptr == other.stub_ptr;
    }

    bool operator==(const MethodCallback<ArgTypes...> &other) const
    {
        return inst_ptr == other.inst_ptr && stub_ptr == other.stub_ptr;
    }
    bool operator!=(const MethodCallback<ArgTypes...> &other) const
    {
        return inst_ptr != other.inst_ptr || stub_ptr != other.stub_ptr;
    }

private:
    typedef void (*StubType)(void *inst_ptr, ArgTypes...);

    MethodCallback(void *inst_ptr, StubType stub_ptr)
        : inst_ptr(inst_ptr)
        , stub_ptr(stub_ptr)
    {}

    static void dummy_stub(void *, ArgTypes...) {}

    template <void (*Function)(ArgTypes...)>
    static void function_stub(void *inst_ptr, ArgTypes... args)
    {
        (void) inst_ptr;
        (*Function)(std::forward<ArgTypes>(args)...);
    }

    template <class ClassType, void (ClassType::*Method)(ArgTypes...)>
    static void method_stub(void *inst_ptr, ArgTypes... args)
    {
        ClassType* inst = static_cast<ClassType *>(inst_ptr);
        (inst->*Method)(std::forward<ArgTypes>(args)...);
    }

    void *inst_ptr;
    StubType stub_ptr;
};

}

#endif // JWUTIL_METHODCALLBACK_H

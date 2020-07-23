#ifndef JWUTIL_BASEEXCEPTION_H
#define JWUTIL_BASEEXCEPTION_H

#include <exception>
#include <string>

namespace jw_util {

class BaseException : public std::exception {
public:
    virtual const char *what() const noexcept {
        return msg.c_str();
    }

protected:
    template <typename... ArgTypes>
    BaseException(const std::string &msg)
        : msg(msg)
    {}

    std::string msg;
};

}

#endif // JWUTIL_BASEEXCEPTION_H

#pragma once

#include "jw_util/baseexception.h"

namespace series {

class InvalidParameterException : public jw_util::BaseException {
public:
    InvalidParameterException(const std::string &msg)
        : BaseException(msg)
    {}
};

}

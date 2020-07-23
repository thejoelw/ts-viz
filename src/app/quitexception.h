#pragma once

#include "jw_util/baseexception.h"

namespace app {

class QuitException : public jw_util::BaseException {
public:
    QuitException()
        : BaseException("Quitting normally")
    {}
};

}

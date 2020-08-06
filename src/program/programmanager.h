#pragma once

#include <unordered_map>
#include <string>

#include "rapidjson/include/rapidjson/document.h"

#include "jw_util/baseexception.h"

#include "app/appcontext.h"
#include "program/progobj.h"

namespace program {

class ProgramManager {
public:
    class InvalidProgramException : public jw_util::BaseException {
        friend class ProgramManager;

    private:
        InvalidProgramException(const std::string &msg)
            : BaseException(msg)
        {}
    };

    ProgramManager(app::AppContext &context);

    void recvRecord(const rapidjson::Document &row);

private:
    app::AppContext &context;

    tf::Taskflow taskflow;

    ProgObj makeProgObj(const rapidjson::Value &value);
};

}

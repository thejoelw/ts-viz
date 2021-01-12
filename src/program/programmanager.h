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
    void end();

    bool isRunning() const { return running; }

private:
    app::AppContext &context;

    bool running = true;
    bool hasProgram = false;

    ProgObj makeProgObj(const std::string &path, const rapidjson::Value &value, std::unordered_map<std::string, ProgObj> &cache);
};

}

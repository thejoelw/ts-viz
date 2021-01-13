#pragma once

#include <string>

namespace app {

class Options {
public:
    static const Options &getInstance() {
        return getMutableInstance();
    }
    static Options &getMutableInstance() {
        static Options instance;
        return instance;
    }

    std::string wisdomDir;
    bool requireExistingWisdom = false;
    bool writeWisdom = true;
};

}

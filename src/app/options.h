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

    unsigned int convMinComputeLog2 = 0;

    std::size_t gcMemoryLimit = static_cast<std::size_t>(-1);
    std::size_t printMemoryUsageOutputIndex = static_cast<std::size_t>(-1);
};

}

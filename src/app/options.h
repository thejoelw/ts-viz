#pragma once

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

    bool writeWisdom = true;
};

}

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "rapidjson/include/rapidjson/document.h"

namespace app {

class Options {
public:
    /*
    template <typename ResultType>
    const ResultType &get(const char *name) {
        static std::unordered_map<std::string, ResultType> map;
        std::unordered_map<std::string, ResultType>::iterator found = map.emp
    }




    void updateFromArgs(int argc, char **argv);
    void updateFromObject(const rapidjson::Value::ConstObject &iterable) {
        for (const auto &entry : iterable) {
            update(entry.name.GetString(), entry.value.GetString());
        }
            printf("Type of member %s is %s\n",
                m.name.GetString(), kTypeNames[m.value.GetType()]);
    }
    void update(const char *key, const char *value);
    */

    static const Options &getInstance() {
        return getMutableInstance();
    }

    // TODO: Make private
    static Options &getMutableInstance() {
        static Options instance;
        return instance;
    }

    static void setInstance(Options newInstance);

    enum EmitFormat { None, Json, Binary };

    std::string title;
    std::string wisdomDir;
    bool requireExistingWisdom = false;
    bool writeWisdom = true;

    unsigned int convMinComputeLog2 = 0;

    std::size_t gcMemoryLimit = static_cast<std::size_t>(-1);
    std::size_t printMemoryUsageOutputIndex = static_cast<std::size_t>(-1);
    std::string debugSeriesToFile;

    EmitFormat emitFormat = EmitFormat::None;

    std::vector<std::size_t> meterIndices;

    std::size_t maxFps = 0;

    bool dontExit = false;

private:
//    std::unordered_multimap<std::string, std::function<void(const std::string &value)>> listeners;
};

}

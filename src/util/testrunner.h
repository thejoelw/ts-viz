#pragma once

#include <vector>

namespace SsProtocol {
namespace Config { struct TestRunner; }
}

namespace app { class AppContext; }

namespace util {

class TestRunner {
public:
    static TestRunner &getInstance() {
        static TestRunner instance;
        return instance;
    }

    void run(const SsProtocol::Config::TestRunner *config);

    int registerGameTest(void (*funcPtr)(app::AppContext &)) {
        gameTests.push_back(funcPtr);
        return 0;
    }

private:
    std::vector<void (*)(app::AppContext &)> gameTests;
};

}

#pragma once

#include <vector>

namespace app { class AppContext; }

namespace util {

class TestRunner {
public:
    static TestRunner &getInstance() {
        static TestRunner instance;
        return instance;
    }

    void run();

    int registerTest(void (*funcPtr)(app::AppContext &)) {
        tests.push_back(funcPtr);
        return 0;
    }

private:
    std::vector<void (*)(app::AppContext &)> tests;
};

}

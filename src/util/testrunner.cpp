#include "testrunner.h"

#include "app/appcontext.h"

namespace util {

void TestRunner::run(const SsProtocol::Config::TestRunner *config) {
    (void) config;

    // Lets create a new test context
    app::AppContext testContext;

    for (std::size_t i = 0; i < gameTests.size(); i++) {
        gameTests[i](testContext);
    }

    gameTests.clear();
}

}

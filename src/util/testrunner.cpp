#include "testrunner.h"

#include "app/appcontext.h"

namespace util {

void TestRunner::run() {
    app::AppContext context;

    for (std::size_t i = 0; i < tests.size(); i++) {
        tests[i](context);
    }

    tests.clear();
}

}

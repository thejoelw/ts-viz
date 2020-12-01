#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "app/appcontext.h"
#include "app/mainloop.h"
#include "app/quitexception.h"

#include "stream/filepoller.h"
#include "stream/jsonunwrapper.h"
#include "program/programmanager.h"
#include "stream/inputmanager.h"
#include "util/testrunner.h"

#include "defs/CHUNK_SIZE_LOG2.h"

int main(int argc, char **argv) {
    // Setup context
    app::AppContext context;

    // Add logger to context
    spdlog::set_default_logger(nullptr);
//    spdlog::set_default_logger(spdlog::stderr_color_st(""));
    spdlog::set_default_logger(spdlog::stderr_color_mt(""));
    context.provideInstance(spdlog::default_logger().get());

    spdlog::set_level(spdlog::level::debug);

    // Print some debug info
#ifndef NDEBUG
    context.get<spdlog::logger>().warn("This is a debug build!");
    std::cout << "A pointer is " << (sizeof(void*) * CHAR_BIT) << " bits" << std::endl;
    std::cout << "An unsigned int is " << (sizeof(unsigned int) * CHAR_BIT) << " bits" << std::endl;
#endif

    context.get<spdlog::logger>().info("Running tests...");
    util::TestRunner::getInstance().run();

    context.get<spdlog::logger>().info("Starting...");

    context.get<spdlog::logger>().info("Chunk size log2 is: {}", CHUNK_SIZE_LOG2);

    // Parse command line options
    if (argc != 3) {
        context.get<spdlog::logger>().critical("Invalid command line options. Example: ./ts-viz program.jsons stream.jsons");
        return 1;
    }

    context.get<stream::FilePoller>().addFile<stream::JsonUnwrapper<program::ProgramManager>>(argv[1]);
    context.get<stream::FilePoller>().addFile<stream::JsonUnwrapper<stream::InputManager>>(argv[2]);

    // Run it!!!
    try {
        context.get<app::MainLoop>().run();
    } catch (const app::QuitException &) {
        // Normal exit
        context.get<spdlog::logger>().info("Ending...");
        context.get<spdlog::logger>().info("AppContext type counts: managed={}, total={}", context.getManagedTypeCount(), context.getTotalTypeCount());
        return 0;
    } catch (const std::exception &exception) {
        context.get<spdlog::logger>().critical("Uncaught exception: {}", exception.what());
        return 1;
    }
}

#include <iostream>

#include "argparse/include/argparse/argparse.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "app/appcontext.h"
#include "app/options.h"
#include "app/mainloop.h"
#include "app/quitexception.h"

#include "version.h"
#include "stream/filepoller.h"
#include "stream/jsonunwrapper.h"
#include "program/programmanager.h"
#include "stream/inputmanager.h"
#include "util/testrunner.h"

#include "defs/CHUNK_SIZE_LOG2.h"

int main(int argc, char **argv) {
    // Setup argument parser
    argparse::ArgumentParser program("ts-viz", tsVizVersion);
    program.add_description("A time series visualizer and processor");

    program.add_argument("program-path")
            .help("The path to the program file or stream")
            .required();

    program.add_argument("data-path")
            .help("The path to the data file or stream")
            .required();

    program.add_argument("--dont-write-wisdom")
            .help("Disable writing fftw's wisdom files")
            .default_value(false)
            .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    app::Options::getMutableInstance().writeWisdom = !program.get<bool>("--dont-write-wisdom");

    // Setup logger
    spdlog::set_default_logger(nullptr);
//    spdlog::set_default_logger(spdlog::stderr_color_st(""));
    spdlog::set_default_logger(spdlog::stderr_color_mt(""));
    spdlog::set_level(spdlog::level::debug);

    // Setup context
    app::AppContext context;

    // Print some debug info
#ifndef NDEBUG
    spdlog::warn("This is a debug build!");
#else
    spdlog::info("This is a release build!");
#endif

    spdlog::info("Version is {}", tsVizVersion);

    spdlog::info("A pointer is {} bits", sizeof(void*) * CHAR_BIT);
    spdlog::info("An unsigned int is {} bits", sizeof(unsigned int) * CHAR_BIT);

    spdlog::info("Running tests...");
    util::TestRunner::getInstance().run();

    spdlog::info("Starting...");

    spdlog::info("Chunk size log2 is: {}", CHUNK_SIZE_LOG2);

    context.get<stream::FilePoller>().addFile<stream::JsonUnwrapper<program::ProgramManager>>(program.get<std::string>("program-path"));
    context.get<stream::FilePoller>().addFile<stream::JsonUnwrapper<stream::InputManager>>(program.get<std::string>("data-path"));

    // Run it!!!
    try {
        context.get<app::MainLoop>().run();
    } catch (const app::QuitException &) {
        // Normal exit
    } catch (const std::exception &exception) {
        spdlog::critical("Uncaught exception: {}", exception.what());
        return 1;
    }

    spdlog::info("Ending...");
    spdlog::info("AppContext type counts: managed={}, total={}", context.getManagedTypeCount(), context.getTotalTypeCount());
    return 0;
}

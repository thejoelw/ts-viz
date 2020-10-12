#include "filepoller.h"

#include <unistd.h>
#include <chrono>
#include <csignal>

#include "spdlog/spdlog.h"

namespace {

void signalHandler(int action) {
    (void) action;
}

}

namespace stream {

FilePoller::FilePoller(app::AppContext &context)
    : TickableBase(context)
{
    struct sigaction act = {};
    act.sa_handler = signalHandler;
    act.sa_flags = 0;
    act.sa_mask = 0;
    sigaction(SIGUSR1, &act, nullptr);
}

FilePoller::~FilePoller() {
    // Make them stop
    running = false;

    // Interrupt all the read() calls
    for (File &file : files) {
        pthread_kill(file.thread.native_handle(), SIGUSR1);
    }

    // Wait until all threads stop
    for (File &file : files) {
        file.thread.join();
    }
}

void FilePoller::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    for (File &file : files) {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        Message msg;
        while (file.messages.try_dequeue(msg)) {
            msg.lineDispatcher(context, msg.data, msg.size);

            if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(10)) {
                break;
            }
        }
    }
}

void FilePoller::loop(FilePoller *filePoller, File &file) {
    static constexpr std::size_t initialChunkSize = 1024 * 1024;

    int fileNo = file.path != "-" ? open(file.path.data(), O_RDONLY) : STDIN_FILENO;
    if (fileNo == -1) {
        spdlog::error("Syscall open() returned -1 and set errno == {}", errno);
        return;
    }

    std::size_t chunkSize = initialChunkSize;
    char *data = new char[chunkSize];
    std::size_t lineStart = 0;
    std::size_t index = 0;

    while (filePoller->running) {
        ssize_t readBytes = read(fileNo, data + index, chunkSize - index);

        if (readBytes == -1) {
            if (errno == EINTR) {
                continue;
            }
            spdlog::error("Syscall read() returned -1 and set errno == {}", errno);
            break;
        } else if (readBytes == 0) {
            break;
        }
        assert(readBytes > 0);

        for (std::size_t end = index + readBytes; index < end; index++) {
            if (data[index] == '\n') {
                Message msg;
                msg.lineDispatcher = file.lineDispatcher;
                msg.data = data + lineStart;
                msg.size = index - lineStart;
                file.messages.enqueue(msg);

                lineStart = index + 1;
            }
        }

        assert(index <= chunkSize);
        if (index == chunkSize) {
            chunkSize = (chunkSize - lineStart) * 2;
            if (chunkSize < initialChunkSize) {
                chunkSize = initialChunkSize;
            } else if (chunkSize > SSIZE_MAX) {
                chunkSize = SSIZE_MAX;
            }

            char *newData = new char[chunkSize];
            std::copy(data + lineStart, data + index, newData);

            Message msg;
            msg.lineDispatcher = &freeer;
            msg.data = data;
            file.messages.enqueue(msg);

            data = newData;
            index -= lineStart;
            lineStart = 0;
        }
    }

    Message msg;
    msg.lineDispatcher = &freeer;
    msg.data = data;
    file.messages.enqueue(msg);

    if (file.path != "-") {
        close(fileNo);
    }
}

}

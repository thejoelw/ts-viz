#include "filepoller.h"

#include <unistd.h>
#include <chrono>
#include <csignal>

#include "log.h"

#include "defs/ENABLE_FILEPOLLER_YIELD_KEYWORD.h"
#include "defs/FILEPOLLER_TICK_TIMEOUT_MS.h"
#include "defs/FILEPOLLER_MAX_QUEUE_SIZE.h"

#include "app/mainloop.h"

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
    sigemptyset(&act.sa_mask);
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
#if FILEPOLLER_TICK_TIMEOUT_MS
        std::chrono::steady_clock::time_point timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(FILEPOLLER_TICK_TIMEOUT_MS);
#endif

        Message msg;
        while (true) {
            if (!file.messages.try_dequeue(msg)) {
#if ENABLE_FILEPOLLER_BLOCKING
                if (!file.blocking) {
                    break;
                }
                std::chrono::steady_clock::duration wait = context.get<app::MainLoop>().getBlockDuration();
                if (wait == std::chrono::steady_clock::duration::zero()) {
                    break;
                }
                if (!file.messages.wait_dequeue_timed(msg, wait)) {
                    break;
                }
#else
                break;
#endif
            }

            static constexpr const char *yieldKeyword = "yield";
            if (ENABLE_FILEPOLLER_YIELD_KEYWORD && msg.size == 5 && std::equal(yieldKeyword, yieldKeyword + 5, msg.data)) {
                break;
            }

            msg.dispatch(context, msg.data, msg.size);

#if FILEPOLLER_TICK_TIMEOUT_MS
            if (std::chrono::steady_clock::now() > timeout) {
                break;
            }
#endif
        }

        file.yieldDispatcher(context);
    }
}

void FilePoller::loop(FilePoller *filePoller, File &file) {
    static constexpr std::size_t initialChunkSize = 1024 * 1024;

    int fileNo = file.path != "-" ? open(file.path.data(), O_RDONLY) : STDIN_FILENO;
    if (fileNo == -1) {
        SPDLOG_ERROR("Syscall open() returned -1 and set errno == {}", errno);
        return;
    }

    std::size_t chunkSize = initialChunkSize;
    char *data = new char[chunkSize];
    std::size_t lineStart = 0;
    std::size_t index = 0;
    std::size_t queuePendingSize = 0;

    while (filePoller->running) {
        ssize_t readBytes = read(fileNo, data + index, chunkSize - index);

        if (readBytes == -1) {
            if (errno == EINTR) {
                continue;
            }
            SPDLOG_ERROR("Syscall read() returned -1 and set errno == {}", errno);
            break;
        } else if (readBytes == 0) {
            break;
        }
        assert(readBytes > 0);

        for (std::size_t end = index + readBytes; index < end; index++) {
            if (data[index] == '\n') {
                file.messages.enqueue(Message(file.lineDispatcher, data + lineStart, index - lineStart));
                lineStart = index + 1;

                queuePendingSize++;
                if (queuePendingSize > FILEPOLLER_MAX_QUEUE_SIZE) {
                    queuePendingSize = file.messages.size_approx();
                    if (queuePendingSize > FILEPOLLER_MAX_QUEUE_SIZE) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
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

            file.messages.enqueue(Message(&freeer, data, 0));

            data = newData;
            index -= lineStart;
            lineStart = 0;
        }
    }

    file.messages.enqueue(Message(&freeer, data, 0));

    if (file.path != "-") {
        close(fileNo);
    }

    file.messages.enqueue(Message(file.endDispatcher, 0, 0));
}

}

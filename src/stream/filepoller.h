#pragma once

#include <deque>
#include <thread>

#include "readerwriterqueue/readerwriterqueue.h"

#include "app/appcontext.h"
#include "app/tickercontext.h"

#include "defs/ENABLE_FILEPOLLER_BLOCKING.h"

namespace stream {

class FilePoller : public app::TickerContext::TickableBase<FilePoller> {
public:
    FilePoller(app::AppContext &context);
    ~FilePoller();

    // Only one file should be blocking; we want it to be the most important one because it'll respond most quickly to new records
    template <typename ReceiverClass>
    void addFile(ReceiverClass &receiver, const std::string &path) {
        File &file = files.emplace_back();
        file.path = path;
        file.receiver = static_cast<void *>(&receiver);
        file.lineDispatcher = &dispatchLine<ReceiverClass>;
        file.yieldDispatcher = &dispatchYield<ReceiverClass>;
        file.endDispatcher = &dispatchEnd<ReceiverClass>;
        file.thread = std::thread(loop, this, std::ref(file));
    }

    void tick(app::TickerContext &tickerContext);

private:
    struct Message {
        Message() {}

        Message(void (*dispatch)(void *receiver, const char *data, std::size_t size), void *receiver, const char *data, std::size_t size)
            : dispatch(dispatch)
            , receiver(receiver)
            , data(data)
            , size(size)
        {}

        void (*dispatch)(void *receiver, const char *data, std::size_t size);
        void *receiver;
        const char *data;
        std::size_t size;
    };

    struct File {
        std::string path;
        void *receiver;
        void (*lineDispatcher)(void *receiver, const char *data, std::size_t size);
        void (*yieldDispatcher)(void *receiver);
        void (*endDispatcher)(void *receiver, const char *data, std::size_t size);

        std::thread thread;
    };
    std::deque<File> files;
    std::atomic<bool> running = true;

#if ENABLE_FILEPOLLER_BLOCKING
        moodycamel::BlockingReaderWriterQueue<Message> messages;
#else
        moodycamel::ReaderWriterQueue<Message> messages;
#endif

    template <typename ReceiverClass>
    static void dispatchLine(void *receiver, const char *data, std::size_t size) {
        static_cast<ReceiverClass *>(receiver)->recvLine(data, size);
    }

    template <typename ReceiverClass>
    static void dispatchYield(void *receiver) {
        static_cast<ReceiverClass *>(receiver)->yield();
    }

    template <typename ReceiverClass>
    static void dispatchEnd(void *receiver, const char *data, std::size_t size) {
        (void) data;
        (void) size;
        static_cast<ReceiverClass *>(receiver)->end();
    }

    static void freeer(void *receiver, const char *data, std::size_t size) {
        (void) receiver;
        (void) size;
        delete[] data;
    }

    static void loop(FilePoller *filePoller, File &threadCtx);
};

}

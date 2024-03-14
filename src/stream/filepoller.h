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

    template <typename ReceiverClass>
    void addFile(const std::string &path, bool blocking) {
        File &file = files.emplace_back();
        file.path = path;
        file.lineDispatcher = &dispatchLine<ReceiverClass>;
        file.yieldDispatcher = &dispatchYield<ReceiverClass>;
        file.endDispatcher = &dispatchEnd<ReceiverClass>;
        file.thread = std::thread(loop, this, std::ref(file));
#if ENABLE_FILEPOLLER_BLOCKING
        file.blocking = blocking;
#endif
    }

    void tick(app::TickerContext &tickerContext);

private:
    struct Message {
        Message() {}

        Message(void (*dispatch)(app::AppContext &context, const char *data, std::size_t size), const char *data, std::size_t size)
            : dispatch(dispatch)
            , data(data)
            , size(size)
        {}

        void (*dispatch)(app::AppContext &context, const char *data, std::size_t size);
        const char *data;
        std::size_t size;
    };

    struct File {
        std::string path;
        void (*lineDispatcher)(app::AppContext &context, const char *data, std::size_t size);
        void (*yieldDispatcher)(app::AppContext &context);
        void (*endDispatcher)(app::AppContext &context, const char *data, std::size_t size);

        std::thread thread;
#if ENABLE_FILEPOLLER_BLOCKING
        moodycamel::BlockingReaderWriterQueue<Message> messages;
        bool blocking;
#else
        moodycamel::ReaderWriterQueue<Message> messages;
#endif
    };
    std::deque<File> files;
    std::atomic<bool> running = true;

    template <typename ReceiverClass>
    static void dispatchLine(app::AppContext &context, const char *data, std::size_t size) {
        context.get<ReceiverClass>().recvLine(data, size);
    }

    template <typename ReceiverClass>
    static void dispatchYield(app::AppContext &context) {
        context.get<ReceiverClass>().yield();
    }

    template <typename ReceiverClass>
    static void dispatchEnd(app::AppContext &context, const char *data, std::size_t size) {
        (void) data;
        (void) size;
        context.get<ReceiverClass>().end();
    }

    static void freeer(app::AppContext &context, const char *data, std::size_t size) {
        (void) context;
        (void) size;
        delete[] data;
    }

    static void loop(FilePoller *filePoller, File &threadCtx);
};

}

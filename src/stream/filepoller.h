#pragma once

#include <deque>
#include <thread>

#include "readerwriterqueue/readerwriterqueue.h"

#include "app/appcontext.h"
#include "app/tickercontext.h"

namespace stream {

class FilePoller : public app::TickerContext::TickableBase<FilePoller> {
public:
    FilePoller(app::AppContext &context);
    ~FilePoller();

    template <typename ReceiverClass>
    void addFile(const std::string &path) {
        File &file = files.emplace_back();
        file.path = path;
        file.lineDispatcher = &dispatchLine<ReceiverClass>;
        file.endDispatcher = &dispatchEnd<ReceiverClass>;
        file.thread = std::thread(loop, this, std::ref(file));
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
        void (*endDispatcher)(app::AppContext &context, const char *data, std::size_t size);

        std::thread thread;
        moodycamel::ReaderWriterQueue<Message> messages;
    };
    std::deque<File> files;
    std::atomic<bool> running = true;

    template <typename ReceiverClass>
    static void dispatchLine(app::AppContext &context, const char *data, std::size_t size) {
        context.get<ReceiverClass>().recvLine(data, size);
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

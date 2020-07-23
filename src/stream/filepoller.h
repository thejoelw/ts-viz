#pragma once

#include <stdio.h>

#include "app/appcontext.h"
#include "app/tickercontext.h"

namespace stream {

class FilePoller : public app::TickerContext::TickableBase<FilePoller> {
public:
    FilePoller(app::AppContext &context);
    ~FilePoller();

    template <typename ReceiverClass>
    void addFile(const char *path) {
        File file;
        file.filePtr = strcmp(path, "-") == 0 ? stdin : fopen(path, "r");
        file.fileNo = fileno(file.filePtr);
        file.lineDispaatcher = &dispatchLine<ReceiverClass>;
        files.push_back(file);
    }

    void tick(app::TickerContext &tickerContext);

private:
    struct File {
        bool closed = false;

        FILE *filePtr;
        int fileNo;

        char *lineData = 0;
        std::size_t lineSize = 0;

        void (*lineDispaatcher)(app::AppContext &context, const char *data, std::size_t size);
    };
    std::vector<File> files;

    template <typename ReceiverClass>
    static void dispatchLine(app::AppContext &context, const char *data, std::size_t size) {
        context.get<ReceiverClass>().recvLine(data, size);
    }
};

}

#include "filepoller.h"

#include <sys/select.h>

namespace stream {

FilePoller::FilePoller(app::AppContext &context)
    : TickableBase(context)
{}

FilePoller::~FilePoller() {
    for (const File &file : files) {
        fclose(file.filePtr);
        free(file.lineData);
    }
}

void FilePoller::tick(app::TickerContext &tickerContext) {
    (void) tickerContext;

    fd_set fds;
    FD_ZERO(&fds);

    int maxFd = files.front().fileNo;
    for (const File &file : files) {
        if (file.closed) {continue;}

        FD_SET(file.fileNo, &fds);

        if (file.fileNo > maxFd) {
            maxFd = file.fileNo;
        }
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    select(maxFd + 1, &fds, NULL, NULL, &tv);

    for (File &file : files) {
        if (file.closed) {continue;}

        if (FD_ISSET(file.fileNo, &fds)) {
            ssize_t size = getline(&file.lineData, &file.lineSize, file.filePtr);
            if (size != -1) {
                file.lineDispaatcher(context, file.lineData, size);
            } else {
                file.closed = true;
            }
        }
    }
}

}

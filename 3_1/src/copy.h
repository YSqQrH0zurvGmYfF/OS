#ifndef COPY_H
#define COPY_H

enum {
    COPY_SUCCESS = 0,
    COPY_FAILURE = -1,
    COPY_MODE_CHANGE_FAILURE = -2,
    COPY_OPEN_FAILURE = -3,
    COPY_IO_FAILURE = -4,
    COPY_INVALID_ARGUMENT = -6,
    COPY_NOT_FOUND = -7
};

int copy_file(const char* src, const char* dst, int mode);
int mkdir_with_mode(const char* dir, int mode);

#endif

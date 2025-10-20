#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdarg.h>

enum return_code {
    GLIBC_FUNC_ERROR = -1,
    SUCCESS = 0,
    ERROR = 1,
    INVALID_PARAMS
};

static inline void
print_log(const char *file, int line, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "[%s:%d] ", file, line);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

#define PRINT_LOG(fmt, ...) print_log(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif

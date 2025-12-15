#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static inline size_t log_append_time(char* str, size_t size) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    size_t len = strftime(str, size, "%H:%M:%S ", tm);
    return len;
}

static inline size_t log_append_location(char* str, size_t size,
                                         const char *file, int line) {
    int n = snprintf(str, size, "\x1b[90m%s:%d\x1b[0m ", file, line);
    return (size_t)n;
}

void log_print(const char *file, int line, const char *fmt, ...) {
    char buf[256];
    size_t size = sizeof(buf);
    size_t len = 0;

    len += log_append_time(buf, size);
    len += log_append_location(buf + len, size - len, file, line);

    if (len < size - 2) {
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf + len, size - len, fmt, args);
        va_end(args);

        if (n > 0) {
            len += n;
            if (len > size - 2) {
                len = size - 2;
            }
        }
    }

    buf[len++] = '\n';
    buf[len]   = '\0';

    fputs(buf, stderr);
}



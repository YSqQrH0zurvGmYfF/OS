#ifndef LOG_H
#define LOG_H

void log_print(const char *file, int line, const char *fmt, ...);

#define PRINT_LOG(fmt, ...) log_print(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif

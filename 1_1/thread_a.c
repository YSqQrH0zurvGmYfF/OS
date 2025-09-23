#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PRINT_LOG(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%d] " fmt "\n", \
        __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

enum return_code {
    SUCCESS = 0,
    ERROR = 1
};

void* mythread(void* arg) {
    size_t thread_num = (size_t) arg;
    printf("thread %zu | %7d | %7d | %7d | %7lu\n",
           thread_num, getpid(), getppid(), gettid(), pthread_self());
    return NULL;
}

int main(void) {
    puts  ("  thread |     pid |    ppid |     tid");
    puts  ("---------|---------|---------|--------");
    printf("    main | %7d | %7d | %7d\n", getpid(), getppid(), gettid());

    pthread_t tid;

    int status = pthread_create(&tid, NULL, mythread, NULL);
    if (status != SUCCESS) {
        PRINT_LOG("Failed to create thread");
        return ERROR;
    }

    pthread_exit(NULL);
    return SUCCESS;
}


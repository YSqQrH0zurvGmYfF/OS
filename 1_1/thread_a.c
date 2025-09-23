#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
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
    printf("  thread | %7d | %7d | %7d\n", getpid(), getppid(), gettid());
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

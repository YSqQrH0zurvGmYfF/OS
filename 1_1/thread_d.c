#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define THREAD_NUM 5

#define PRINT_LOG(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%d] " fmt "\n", \
        __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)


enum return_code {
    SUCCESS = 0,
    ERROR = 1
};


int global_value = 0;

void* mythread(void* arg) {
    size_t thread_num = (size_t) arg;

    int local_value = 0;

    printf("thread %zu : local_value=%d;global_value=%d\n",
           thread_num, local_value, global_value);

    local_value++;
    global_value++;

    printf("thread %zu : local_value=%d;global_value=%d\n",
           thread_num, local_value, global_value);

    sleep(12345);
    return NULL;
}

int main(void) {
    pthread_t tid[THREAD_NUM];

    for (size_t i = 0; i < THREAD_NUM; i++) {
        int status = pthread_create(&tid[i], NULL, mythread, (void*) i);
        if (status != SUCCESS) {
            PRINT_LOG("Failed to create thread %zu : %s", i, strerror(status));
            return ERROR;
        }
    }

    pthread_exit(NULL);
    return SUCCESS;
}


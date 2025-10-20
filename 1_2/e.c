#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

void* mythread(void* arg) {
    size_t thread_num = (size_t) arg;
    printf("My thread_num = %7zu and tid = %lu\n", thread_num, pthread_self());
    
    int err = pthread_detach(pthread_self());
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to detach to %zu : %s\n", thread_num, strerror(err));
    }
    return NULL;
}

int main(void) {
    pthread_t tid;

    for (size_t i = 0;; i++) {
        int err = pthread_create(&tid, NULL, mythread, (void*) i);
        if (err != SUCCESS) {
            fprintf(stderr, "Failed to create thread %zu : %s\n", i, strerror(err));
            break;
        }
    }

    return EXIT_SUCCESS;
}


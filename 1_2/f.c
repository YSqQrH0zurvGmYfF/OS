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
    return NULL;
}

int main(void) {
    int err;

    pthread_attr_t detach_attr;
    err = pthread_attr_init(&detach_attr);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to init attribute : %s", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_attr_setdetachstate(&detach_attr,PTHREAD_CREATE_DETACHED);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to set DETACHED to attribute : %s", strerror(err));
        pthread_attr_destroy(&detach_attr);
        return EXIT_FAILURE;
    }

    pthread_t tid;

    for (size_t i = 0;; i++) {
        int err = pthread_create(&tid, &detach_attr, mythread, (void*) i);
        if (err != SUCCESS) {
            fprintf(stderr, "Failed to create thread %zu : %s\n", i, strerror(err));
            break;
        }
    }

    pthread_attr_destroy(&detach_attr);
    return EXIT_SUCCESS;
}


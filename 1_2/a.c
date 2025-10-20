#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

void* mythread(void* arg) {
    return NULL;
}

int main(void) {
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to create thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_join(tid, NULL);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to join thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


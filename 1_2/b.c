#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

void* mythread(void* arg) {
    size_t result = 42;
    return (void*) result;
}

int main(void) {
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to create thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    size_t thread_result;
    err = pthread_join(tid, (void**) &thread_result);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to join thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    printf("Hey. I've got %zu.\n", thread_result);  
    return EXIT_SUCCESS;
}


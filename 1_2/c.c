#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

static const char* MSG = "hello world";

void* mythread(void* arg) {
    char* str = malloc(sizeof(char) * (strlen(MSG) + 1));
    strcpy(str, MSG);
    return str;
}

int main(void) {
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to create thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    char* thread_result;
    err = pthread_join(tid, (void**) &thread_result);
    if (err != SUCCESS) {
        fprintf(stderr, "Failed to join thread : %s\n", strerror(err));
        return EXIT_FAILURE;
    }
    
    puts(thread_result);
    free(thread_result);
    return EXIT_SUCCESS;
}


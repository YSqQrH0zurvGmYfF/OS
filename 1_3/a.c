#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"


typedef struct {
    int num;
    char* str;
} data_t;


void* mythread(void* arg) {
    data_t* data = (data_t*) arg;
    printf("num = %d\nstring = %s\n", data->num, data->str);
    return NULL;
}


int main(void) {
    int err;

    data_t data = {
        .num = 123,
        .str = "hello world"
    };

    pthread_t tid;
    err = pthread_create(&tid, NULL, mythread, &data);
    if (err != SUCCESS) {
        perror("Failed to create thread");
        return EXIT_FAILURE;
    }

    err = pthread_join(tid, NULL);
    if (err != SUCCESS) {
        perror("Failed to join thread");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

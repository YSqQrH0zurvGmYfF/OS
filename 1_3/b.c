#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

static const char* MSG = "Hello world";

typedef struct {
    int num;
    char* str;
} data_t;

void* mythread(void* arg) {
    data_t* data = (data_t*) arg;
    printf("num = %d\nstring = %s\n", data->num, data->str);

    free(data->str);
    free(data);
    return NULL;
}

int init_detached_attr(pthread_attr_t* attr) {
    int err;
    err = pthread_attr_init(attr);
    if (err != SUCCESS) {
        perror("Pthreads attribute init failed");
        return ERROR;
    }
    
    err = pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
    if (err != SUCCESS) {
        perror("Failed to set detach state for Pthreads attribute");
        pthread_attr_destroy(attr);
        return ERROR;
    }

    return SUCCESS;
}

static data_t* init_data(void) {
    data_t* data = malloc(sizeof(data_t));

    if (data == NULL) {
        printf("Failed to allocate memory for structure\n");
        return NULL;
    }

    data->num = 123;
    data->str = malloc(strlen(MSG) + 1); // +1 to include '\0'

    if (data->str == NULL) {
        printf("Failed to allocate memory for string");
        free(data);
        return NULL;
    }

    strcpy(data->str, MSG);
    return data;
}


int main(void) {
    int err;

    pthread_attr_t attr;
    int init_attr_return_val = init_detached_attr(&attr);
    if (init_attr_return_val == ERROR) {
        return EXIT_FAILURE;
    }

    data_t* data = init_data();
    if (data == NULL) {
        pthread_attr_destroy(&attr);
        return EXIT_FAILURE;
    }

    pthread_t tid;
    err = pthread_create(&tid, &attr, mythread, data);

    if (err != SUCCESS) {
        perror("Failed to create thread");
        pthread_attr_destroy(&attr);
        free(data->str);
        free(data);
        return EXIT_FAILURE;
    }

    pthread_attr_destroy(&attr);
    
    pthread_exit(NULL);
    return EXIT_FAILURE;
}

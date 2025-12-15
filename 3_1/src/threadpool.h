#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>

enum {
    TP_SUCCESS = 0,
    TP_FAILURE = -1,
    TP_ALLOCATION_FAILURE = -2,
    TP_INVALID_ARGUMENT = -3,
    TP_LOCK_FAILED = -4,
    TP_CANCELED_BY_DESTROY = -5,
    TP_THREAD_START_FAILURE = -6
};

typedef struct tp tp_t;

typedef struct {
    size_t thread_num;
    size_t queue_len;

    void (*handler)(void*);
} tp_conf_t;

int tp_init(tp_t** p, const tp_conf_t* conf);

// Graceful destroy
int tp_destroy(tp_t* pool);

int tp_add(tp_t* pool, void* task);

int tp_wait_idling(tp_t* pool);

#endif /* THREADPOOL_H */

#include "threadpool.h"

#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define SUCCESS 0

struct tp {
    pthread_t* threads;
    size_t thread_num;

    void (*handler)(void*);

    size_t inactive_thread_num;

    void** queue;
    size_t queue_len;

    size_t queue_num;
    size_t queue_head;
    size_t queue_tail;

    pthread_mutex_t lock;
    pthread_cond_t notify;

    int shutdown;
};

static void* tp_thread(void* tp) {
    int status;

    tp_t* pool = (tp_t*)tp;

    while(1) {
        status = pthread_mutex_lock(&pool->lock);
        if (status != SUCCESS) { break; }

        pool->inactive_thread_num++;
        pthread_cond_broadcast(&pool->notify);

        while((pool->queue_num == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        void* task = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_len;
        pool->queue_num -= 1;
        pool->inactive_thread_num--;

        pthread_cond_broadcast(&pool->notify);
        pthread_mutex_unlock(&pool->lock);

        pool->handler(task);
    }
    return NULL;
}

static inline tp_t* tp_alloc(size_t thread_num, size_t queue_size) {
    tp_t* pool;

    pool = calloc(1, sizeof(*pool));
    if (pool == NULL) { return pool; }

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);

    pool->queue = malloc(sizeof(*pool->queue) * queue_size);
    if (pool->queue == NULL) { goto free_pool; }

    pool->threads = malloc(sizeof(*pool->threads) * thread_num);
    if (pool->threads == NULL) {
        free(pool->queue);
        goto free_pool;
    }

    return pool;

free_pool:
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool);
    return NULL;
}

int tp_init(tp_t** p, const tp_conf_t* conf) {
    tp_t* pool;

    if (conf == NULL || conf->handler == NULL ||
        conf->thread_num == 0 || conf->queue_len == 0) {
        return TP_INVALID_ARGUMENT;
    }

    pool = tp_alloc(conf->thread_num, conf->queue_len);
    if (pool == NULL) { return TP_ALLOCATION_FAILURE; }

    pool->handler = conf->handler;
    pool->queue_len = conf->queue_len;

    for(size_t i = 0; i < conf->thread_num; i++) {
        int status = pthread_create(&pool->threads[i], NULL,
                                    tp_thread, (void*)pool);
        if(status != SUCCESS) { goto pthread_err; }
        pool->thread_num++;
    }

    *p = pool;
    return TP_SUCCESS;

 pthread_err:
    tp_destroy(pool);    
    return TP_THREAD_START_FAILURE;
}


int tp_destroy(tp_t* pool) {
    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    if (pool->shutdown == 1) {
        pthread_mutex_unlock(&pool->lock);
        return TP_CANCELED_BY_DESTROY;
    }

    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->notify);

    pthread_mutex_unlock(&pool->lock);
       
    for (; pool->thread_num > 0; pool->thread_num--) {
        pthread_join(pool->threads[pool->thread_num - 1], NULL);
    }

    free(pool->queue);
    free(pool->threads);
    
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);

    free(pool);

    return TP_SUCCESS;
}

int tp_add(tp_t* pool, void* task) {
    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    while (pool->queue_num == pool->queue_len && !pool->shutdown) {
        pthread_cond_wait(&pool->notify, &pool->lock);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return TP_CANCELED_BY_DESTROY;
    }

    pool->queue[pool->queue_tail] = task;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_len;
    pool->queue_num += 1;

    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return TP_SUCCESS;
}

int tp_wait_idling(tp_t *pool) {
    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    while (pool->inactive_thread_num != pool->thread_num) {
        pthread_cond_wait(&pool->notify, &pool->lock);
    }
    
    pthread_mutex_unlock(&pool->lock);
    return TP_SUCCESS;
}


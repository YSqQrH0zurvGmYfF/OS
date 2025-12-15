#include "threadpool.h"

#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define SUCCESS 0

typedef struct task_node {
    void* task;
    struct task_node* next;
} task_node_t;

struct tp {
    pthread_t* threads;
    size_t thread_num;

    void (*handler)(void*);

    size_t inactive_thread_num;

    task_node_t* head;
    task_node_t* tail;

    size_t queue_num;

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

        task_node_t* node = pool->head;
        void* task = node->task;
        
        pool->head = node->next;
        if (pool->head == NULL) {
            pool->tail = NULL;
        }

        pool->queue_num--;
        pool->inactive_thread_num--;

        free(node);

        pthread_cond_broadcast(&pool->notify);
        pthread_mutex_unlock(&pool->lock);

        pool->handler(task);
    }
    return NULL;
}

static inline tp_t* tp_alloc(size_t thread_num) {
    tp_t* pool;

    pool = calloc(1, sizeof(*pool));
    if (pool == NULL) { return NULL; }

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);

    pool->queue_num = 0;
    pool->head = NULL;
    pool->tail = NULL;

    pool->threads = malloc(sizeof(*pool->threads) * thread_num);
    if (pool->threads == NULL) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->notify);
        free(pool);
        return NULL;
    }

    return pool;
}

int tp_init(tp_t** p, const tp_conf_t* conf) {
    tp_t* pool;

    if (conf == NULL || conf->handler == NULL || conf->thread_num == 0) {
        return TP_INVALID_ARGUMENT;
    }

    pool = tp_alloc(conf->thread_num);
    if (pool == NULL) { return TP_ALLOCATION_FAILURE; }

    pool->handler = conf->handler;

    for(size_t i = 0; i < conf->thread_num; i++) {
        int status = pthread_create(&pool->threads[i], NULL,
                                    tp_thread, (void*)pool);
        if(status != SUCCESS) { 
            // Handle partial failure
            pool->shutdown = 1; // Signal started threads to stop
            tp_destroy(pool); 
            return TP_THREAD_START_FAILURE; 
        }
        pool->thread_num++;
    }

    *p = pool;
    return TP_SUCCESS;
}

int tp_destroy(tp_t* pool) {
    if (pool == NULL) return TP_INVALID_ARGUMENT;

    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    if (pool->shutdown == 1) {
        pthread_mutex_unlock(&pool->lock);
        return TP_CANCELED_BY_DESTROY;
    }

    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->notify);

    pthread_mutex_unlock(&pool->lock);

    for (size_t i = 0; i < pool->thread_num; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    task_node_t* current = pool->head;
    while (current != NULL) {
        task_node_t* temp = current;
        current = current->next;
        free(temp);
    }

    free(pool->threads);
    
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);

    free(pool);

    return TP_SUCCESS;
}

int tp_add(tp_t* pool, void* task) {
    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->lock);
        return TP_CANCELED_BY_DESTROY;
    }

    task_node_t* new_node = malloc(sizeof(task_node_t));
    if (new_node == NULL) {
        pthread_mutex_unlock(&pool->lock);
        return TP_ALLOCATION_FAILURE;
    }

    new_node->task = task;
    new_node->next = NULL;

    if (pool->tail != NULL) {
        pool->tail->next = new_node;
    } else {
        pool->head = new_node;
    }
    pool->tail = new_node;
    pool->queue_num++;

    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return TP_SUCCESS;
}

int tp_wait_idling(tp_t *pool) {
    int status = pthread_mutex_lock(&pool->lock);
    if (status != SUCCESS) { return TP_LOCK_FAILED; }

    while (pool->queue_num > 0 || pool->inactive_thread_num != pool->thread_num) {
        pthread_cond_wait(&pool->notify, &pool->lock);
    }

    pthread_mutex_unlock(&pool->lock);
    return TP_SUCCESS;
}

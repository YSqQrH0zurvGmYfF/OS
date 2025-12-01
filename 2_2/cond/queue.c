#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void* qmonitor(void* arg) {
	queue_t* q = (queue_t*)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		int status = pthread_mutex_lock(&q->lock);
		if (status != SUCCESS) {
	        printf("qmonitor: pthread_mutex_lock() failed: %s\n",
	               strerror(status));
	        abort();
	    }

		queue_print_stats(q);

		status = pthread_mutex_unlock(&q->lock);
		if (status != SUCCESS) {
	        printf("qmonitor: pthread_mutex_unlock() failed: %s\n",
	               strerror(status));
	        abort();
	    }

		sleep(1);
	}

	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t* q = malloc(sizeof(queue_t));
	if (q == NULL) {
		printf("Cannot allocate memory for a queue\n");
		abort();
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->notify, NULL);

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err != SUCCESS) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}

	return q;
}

void queue_destroy(queue_t* q) {
	int status;

	status = pthread_cancel(q->qmonitor_tid);
    if (status != SUCCESS) {
        printf("queue_destroy: pthread_create() failed: %s\n",
               strerror(status));
        abort();
    }

    status = pthread_join(q->qmonitor_tid, NULL);
    if (status != SUCCESS) {
        printf("queue_destroy: pthread_join() failed: %s\n",
               strerror(status));
        abort();
    }

	status = pthread_mutex_destroy(&q->lock);
	if (status != SUCCESS) {
        printf("queue_destroy: pthread_mutex_destroy() failed: %s\n",
               strerror(status));
        abort();
    }

	status = pthread_cond_destroy(&q->notify);
	if (status != SUCCESS) {
        printf("queue_destroy: pthread_cond_destroy() failed: %s\n",
               strerror(status));
        abort();
    }

    qnode_t* curr = q->first;
    while (curr != NULL) {
        qnode_t *next = curr->next;
        free(curr);
        curr = next;
    }

    free(q);
}

void mutex_unlocker(void* mutex) {
	pthread_mutex_unlock(mutex);
}

int queue_add(queue_t* q, int val) {
	pthread_cleanup_push(mutex_unlocker, &q->lock);
	
	int status = pthread_mutex_lock(&q->lock);
	if (status != SUCCESS) {
        printf("queue_add: pthread_mutex_lock() failed: %s\n",
               strerror(status));
        abort();
    }

	q->add_attempts++;

	assert(q->count <= q->max_count);

	while (q->count == q->max_count) {
		pthread_cond_wait(&q->notify, &q->lock);
	}

	qnode_t* new = malloc(sizeof(qnode_t));
	if (new == NULL) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}

	new->val = val;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	pthread_cond_broadcast(&q->notify);

    pthread_cleanup_pop(1);
	return SUCCESS;
}

int queue_get(queue_t* q, int* val) {
	pthread_cleanup_push(mutex_unlocker, &q->lock);
	
	int status = pthread_mutex_lock(&q->lock);
	if (status != SUCCESS) {
        printf("queue_get: pthread_mutex_lock() failed: %s\n",
               strerror(status));
        abort();
    }

    q->get_attempts++;

	assert(q->count >= 0);

	while (q->count == 0) {
		pthread_cond_wait(&q->notify, &q->lock);
	}

	qnode_t* tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	pthread_cond_broadcast(&q->notify);

    pthread_cleanup_pop(1);
	return SUCCESS;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
}


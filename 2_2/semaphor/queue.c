#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "queue.h"

void* qmonitor(void* arg) {
	queue_t* q = (queue_t*)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		int status = sem_wait(&q->lock);
		if (status != SUCCESS) {
	        printf("queue_add: sem_wait() failed: %s\n",
	               strerror(status));
	        abort();
	    }

		queue_print_stats(q);

		status = sem_post(&q->lock);
		if (status != SUCCESS) {
	        printf("queue_add: sem_wait() failed: %s\n",
	               strerror(status));
	        abort();
	    }

		sleep(1);
	}

	return NULL;
}

queue_t* queue_init(int max_count) {
	int status;

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

	int process_shared = 0;
	status = sem_init(&q->emptiness, process_shared, max_count);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() failed: %s\n", strerror(status));
		abort();
	}
	status = sem_init(&q->fullness, process_shared, 0);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() failed: %s\n", strerror(status));
		abort();
	}
	status = sem_init(&q->lock, process_shared, 1);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() failed: %s\n", strerror(status));
		abort();
	}

	status = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (status != SUCCESS) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(status));
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

	status = sem_destroy(&q->lock);
	if (status != SUCCESS) {
        printf("queue_destroy: sem_destroy() failed: %s\n",
               strerror(status));
        abort();
    }

	status = sem_destroy(&q->emptiness);
	if (status != SUCCESS) {
        printf("queue_destroy: sem_destroy() failed: %s\n",
               strerror(status));
        abort();
    }

    status = sem_destroy(&q->fullness);
	if (status != SUCCESS) {
        printf("queue_destroy: sem_destroy() failed: %s\n",
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

int queue_add(queue_t* q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

	int status = sem_wait(&q->emptiness);
	if (status != SUCCESS) {
        printf("queue_add: sem_wait() failed: %s\n",
               strerror(status));
        abort();
    }

	status = sem_wait(&q->lock);
	if (status != SUCCESS) {
        printf("queue_add: sem_wait() failed: %s\n",
               strerror(status));
        abort();
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

	status = sem_post(&q->lock);
	if (status != SUCCESS) {
        printf("queue_add: sem_post() failed: %s\n",
               strerror(status));
        abort();
    }

	status = sem_post(&q->fullness);
	if (status != SUCCESS) {
        printf("queue_add: sem_post() failed: %s\n",
               strerror(status));
        abort();
    }

	return SUCCESS;
}

int queue_get(queue_t* q, int* val) {
    q->get_attempts++;

	assert(q->count >= 0);


	int status = sem_wait(&q->fullness);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() failed: %s\n",
               strerror(status));
        abort();
    }

    status = sem_wait(&q->lock);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() failed: %s\n",
               strerror(status));
        abort();
    }

	qnode_t* tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	status = sem_post(&q->lock);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() failed: %s\n",
               strerror(status));
        abort();
    }

	status = sem_post(&q->emptiness);
	if (status != SUCCESS) {
        printf("queue_get: sem_post() failed: %s\n",
               strerror(status));
        abort();
    }

	return SUCCESS;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
}


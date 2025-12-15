#define _GNU_SOURCE
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <assert.h>

#include "queue.h"

static void* qmonitor(void* arg) {
	queue_t* q = (queue_t*)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		queue_print_stats(q);
		sleep(1);
	}

	return NULL;
}

static int queue_sem_init(queue_t* q, unsigned max_count) {
	int status;
	int process_shared = 0;

	status = sem_init(&q->emptiness, process_shared, max_count);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() on emptiness failed: %s\n", strerror(errno));
		return ERROR;
	}
	status = sem_init(&q->fullness, process_shared, 0);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() on fullness failed: %s\n", strerror(errno));
		sem_destroy(&q->emptiness);
		return ERROR;
	}
	status = sem_init(&q->lock, process_shared, 1);
	if (status != SUCCESS) {
		printf("queue_init: sem_init() on lock failed: %s\n", strerror(errno));
		sem_destroy(&q->emptiness);
		sem_destroy(&q->fullness);
		return ERROR;
	}

	return SUCCESS;
}

static void queue_sem_destroy(queue_t* q) {
	int status;

	status = sem_destroy(&q->lock);
	if (status != SUCCESS) {
		printf("queue_destroy: sem_destroy() on lock failed: %s\n",
		       strerror(errno));
	}

	status = sem_destroy(&q->emptiness);
	if (status != SUCCESS) {
		printf("queue_destroy: sem_destroy() on emptiness failed: %s\n",
		       strerror(errno));
	}

	status = sem_destroy(&q->fullness);
	if (status != SUCCESS) {
		printf("queue_destroy: sem_destroy() on fullness failed: %s\n",
		       strerror(errno));
	}
}

queue_t* queue_init(int max_count) {
	int status;

	queue_t* q = malloc(sizeof(queue_t));
	if (q == NULL) {
		printf("queue_init: malloc() failed: %s\n", strerror(errno));
		return NULL;
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	status = queue_sem_init(q, max_count);
	if (status != SUCCESS) {
		free(q);
		return NULL;
	}

	status = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (status != SUCCESS) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(status));
		queue_sem_destroy(q);
		free(q);
		return NULL;
	}

	return q;
}

static void cleanup_sem_post(void* sem) {
	sem_post(sem);
}

int queue_destroy(queue_t* q) {
	int status;

	status = pthread_cancel(q->qmonitor_tid);
    if (status != SUCCESS) {
        printf("queue_destroy: pthread_create() failed: %s\n",
               strerror(status));
        return ERROR;
    }

    status = pthread_join(q->qmonitor_tid, NULL);
    if (status != SUCCESS) {
        printf("queue_destroy: pthread_join() failed: %s\n",
               strerror(status));
        return ERROR;
    }

    queue_sem_destroy(q);
	
    qnode_t* curr = q->first;
    while (curr != NULL) {
        qnode_t *next = curr->next;
        free(curr);
        curr = next;
    }

    free(q);

    return SUCCESS;
}

static inline int queue_add_impl(queue_t* q, int val) {
	qnode_t* new = malloc(sizeof(qnode_t));
	if (new == NULL) {
		printf("queue_add: malloc() failed: %s\n", strerror(errno));
		return ERROR;
	}

	new->val = val;
	new->next = NULL;

	if (!q->first) {
		q->first = q->last = new;
	} else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	return SUCCESS;
}

int queue_add(queue_t* q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

	int status = sem_wait(&q->emptiness);
	if (status != SUCCESS) {
        printf("queue_add: sem_wait() on emptiness failed: %s\n",
               strerror(errno));
        return ERROR;
    }
	pthread_cleanup_push(cleanup_sem_post, &q->emptiness);
    status = sem_wait(&q->lock);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() on lock failed: %s\n",
               strerror(errno));
        sem_post(&q->fullness);
        return ERROR;
    }
    pthread_cleanup_pop(0);

	queue_add_impl(q, val);
	if (status != SUCCESS) {
        sem_post(&q->lock);
        sem_post(&q->emptiness);
        return ERROR;
    }

	sem_post(&q->lock);

	status = sem_post(&q->fullness);
	if (status != SUCCESS) {
        printf("queue_add: sem_post() on fullness failed: %s\n",
               strerror(errno));
    }

	return SUCCESS;
}

int queue_get(queue_t* q, int* val) {
    q->get_attempts++;

	assert(q->count >= 0);

	int status = sem_wait(&q->fullness);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() on fullness failed: %s\n",
               strerror(errno));
        return ERROR;
    }
    pthread_cleanup_push(cleanup_sem_post, &q->fullness);
    status = sem_wait(&q->lock);
	if (status != SUCCESS) {
        printf("queue_get: sem_wait() on lock failed: %s\n",
               strerror(errno));
        sem_post(&q->fullness);
        return ERROR;
    }
    pthread_cleanup_pop(0);

	qnode_t* tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	sem_post(&q->lock);

	status = sem_post(&q->emptiness);
	if (status != SUCCESS) {
        printf("queue_get: sem_post() on emptiness failed: %s\n",
               strerror(errno));
    }

	return SUCCESS;
}

void queue_print_stats(queue_t *q) {
	int status = sem_wait(&q->lock);
	if (status != SUCCESS) {
        printf("queue_add: sem_wait() failed: %s\n",
               strerror(errno));
        return;
    }

	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);

	sem_post(&q->lock);
}


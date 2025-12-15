#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#include "copy.h"
#include "log.h"
#include "threadpool.h"

const size_t DEFAULT_THREAD_NUM = 6;
const size_t DEFAULT_QUEUE_LEN = 2048;


typedef struct {
    int mode;
    char* src_path;
    char* dst_path;

    tp_t* pool;
} task_t;


static void tp_handler(void* arg);


static char* task_path_join(const char* a, const char* b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);

    int need_sep = (la == 0) ? 0 : (a[la-1] != '/');

    size_t total = la + need_sep + lb + 1;
    char* res = malloc(total);
    if (res == NULL) { return NULL; }

    strcpy(res, a);
    if (need_sep) { strcat(res, "/"); }
    strcat(res, b);

    return res;
}

static inline void task_destroy(task_t* task) {
    free(task->src_path);
    free(task->dst_path);
    free(task);
}

static task_t* task_init(const char* src, const char* dst, const char* filename) {
    task_t* task = malloc(sizeof(*task));
    if (task == NULL) {
        PRINT_LOG("Error: task allocation failed for '%s/%s'", src, filename);
        return NULL;
    }

    task->src_path = NULL;
    task->dst_path = NULL;

    if (filename != NULL) {
        task->src_path = task_path_join(src, filename);
        task->dst_path = task_path_join(dst, filename);
    } else {
        task->src_path = strdup(src);
        task->dst_path = strdup(dst);
    }

    if (task->src_path == NULL || task->dst_path == NULL) {
        PRINT_LOG("Error: task allocation failed for '%s/%s'", src, filename);
        task_destroy(task);
        return NULL;
    }

    struct stat st;
    if (lstat(task->src_path, &st) != 0) {
        PRINT_LOG("Error: lstat failed for '%s'", task->src_path);
        task_destroy(task);
        return NULL;
    }

    task->mode = st.st_mode;

    return task;
}

static void process_folder(task_t* task) {
    int status = mkdir_with_mode(task->dst_path, task->mode);
    if (status != COPY_SUCCESS) {
        PRINT_LOG("Error: failed to mkdir '%s'", task->dst_path);
        return;
    }

    DIR* dir = opendir(task->src_path);
    if (dir == NULL) {
        PRINT_LOG("Error: failed to open directory '%s'", task->src_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char* filename = entry->d_name;
        if (!strcmp(".", filename)|| !strcmp("..", filename)) { continue; }

        task_t* new_task = task_init(task->src_path, task->dst_path, filename);
        if (new_task == NULL) { continue; }

        new_task->pool = task->pool;
        tp_add(task->pool, new_task);
    }

    closedir(dir);
}

static void tp_handler(void* arg) {
    task_t* task = arg;

    if (S_ISDIR(task->mode)) {
        process_folder(task);
    } else if (S_ISREG(task->mode)) {
        int status = copy_file(task->src_path, task->dst_path, task->mode);
        if (status == COPY_MODE_CHANGE_FAILURE) {
            PRINT_LOG("Warning: failed to copy mode of '%s' to '%s',"
                      "but data was copied fully",
                      task->src_path, task->dst_path);
        } else if (status != COPY_SUCCESS) {
            PRINT_LOG("Error: failed to create copy of '%s' at '%s': %d",
                      task->src_path, task->dst_path, status);
        }
    } else if (S_ISLNK(task->mode)) {
        PRINT_LOG("Info: ignoring '%s' because this is symlink", task->src_path);
    } else {
        PRINT_LOG("Info: ignoring '%s' because of unsupported file type", task->src_path);
    }

    task_destroy(task);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <src_root> <dst_root>\n", argv[0]);
        return EXIT_FAILURE;
    }

    task_t* first_task = task_init(argv[1], argv[2], NULL);
    if (first_task == NULL) { return EXIT_FAILURE; }

    tp_conf_t conf;
    conf.thread_num = DEFAULT_THREAD_NUM;
    conf.queue_len = DEFAULT_QUEUE_LEN;
    conf.handler = tp_handler;

    tp_t* pool = NULL;
    int rc = tp_init(&pool, &conf);
    if (rc != TP_SUCCESS) {
        task_destroy(first_task);
        PRINT_LOG("Failed to init threadpool: %d\n", rc);
        return EXIT_FAILURE;
    }

    first_task->pool = pool;
    tp_add(pool, first_task);

    int status = tp_wait_idling(pool);
    if (status != TP_SUCCESS) {
        PRINT_LOG("Failed to wait threadpool work ending.");
        return EXIT_FAILURE;
    }

    tp_destroy(pool);
    return EXIT_SUCCESS;
}

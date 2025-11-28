#include "../include/xfs_trans.h"
#include "../include/xfs_types.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

// Barrier sync structure for synchronization
typedef struct barrier_sync {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
} barrier_sync_t;

// Transaction log queue node
typedef struct log_queue_node {
    void *data;
    size_t len;
    int is_barrier;  // 1 if this is a barrier transaction, 0 otherwise
    barrier_sync_t *barrier_sync;  // Sync structure for barrier synchronization
    struct log_queue_node *next;
} log_queue_node_t;

static log_queue_node_t *log_head = NULL;
static log_queue_node_t *log_tail = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;
static int log_worker_running = 0;
static pthread_t log_worker_thread;

// Initialize a barrier sync structure
static barrier_sync_t* barrier_sync_init(void) {
    barrier_sync_t* sync = (barrier_sync_t*)malloc(sizeof(barrier_sync_t));
    if (sync == NULL) {
        return NULL;
    }

    if (pthread_mutex_init(&sync->mutex, NULL) != 0) {
        free(sync);
        return NULL;
    }

    if (pthread_cond_init(&sync->cond, NULL) != 0) {
        pthread_mutex_destroy(&sync->mutex);
        free(sync);
        return NULL;
    }

    sync->signaled = 0;

    return sync;
}

// Destroy a barrier sync structure
static void barrier_sync_destroy(barrier_sync_t* sync) {
    if (sync == NULL) {
        return;
    }

    pthread_cond_destroy(&sync->cond);
    pthread_mutex_destroy(&sync->mutex);
    free(sync);
}

// Signal a barrier sync
static void barrier_sync_signal(barrier_sync_t* sync) {
    if (sync == NULL) {
        return;
    }

    pthread_mutex_lock(&sync->mutex);
    sync->signaled = 1;
    pthread_cond_broadcast(&sync->cond);
    pthread_mutex_unlock(&sync->mutex);
}

// Wait on a barrier sync
static void barrier_sync_wait(barrier_sync_t* sync) {
    if (sync == NULL) {
        return;
    }

    pthread_mutex_lock(&sync->mutex);
    while (!sync->signaled) {
        pthread_cond_wait(&sync->cond, &sync->mutex);
    }
    pthread_mutex_unlock(&sync->mutex);
}

// Log worker function - processes the log queue
static void *log_worker(void *arg) {
    log_queue_node_t *current;

    while (1) {
        // Wait for work
        pthread_mutex_lock(&log_mutex);
        while (log_head == NULL && log_worker_running) {
            pthread_cond_wait(&log_cond, &log_mutex);
        }

        if (!log_worker_running) {
            pthread_mutex_unlock(&log_mutex);
            break;
        }

        // Get the next item in the queue
        current = log_head;
        if (current != NULL) {
            log_head = current->next;
            if (log_head == NULL) {
                log_tail = NULL;
            }
        }
        pthread_mutex_unlock(&log_mutex);

        if (current != NULL) {
            // Simulate writing to disk (this is where the actual "log flush" would happen)
            printf("[System] Flushing transaction to log\n");
            usleep(100000);  // Simulate I/O delay (100ms)

            // If this is a barrier transaction, signal the waiting thread
            if (current->is_barrier && current->barrier_sync) {
                printf("[System] Log Flushed - Signaling barrier\n");
                barrier_sync_signal(current->barrier_sync);
            }

            // Free the transaction data
            if (current->data) {
                free(current->data);
            }
            free(current);
        }
    }

    return NULL;
}

// Initialize the transaction system and log flushing thread
int trans_init(void) {
    log_worker_running = 1;
    
    if (pthread_create(&log_worker_thread, NULL, log_worker, NULL) != 0) {
        log_worker_running = 0;
        return -1;
    }
    
    return 0;
}

// Add a metadata change to the in-memory log queue
int trans_add_item(void* data, int len) {
    log_queue_node_t *node = (log_queue_node_t *)malloc(sizeof(log_queue_node_t));
    if (node == NULL) {
        return -1;
    }

    node->data = malloc(len);
    if (node->data == NULL) {
        free(node);
        return -1;
    }

    memcpy(node->data, data, len);
    node->len = len;
    node->is_barrier = 0;
    node->barrier_sync = NULL;
    node->next = NULL;

    pthread_mutex_lock(&log_mutex);
    if (log_tail == NULL) {
        log_head = log_tail = node;
    } else {
        log_tail->next = node;
        log_tail = node;
    }
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);

    return 0;
}

// Commit a transaction barrier - blocks until the log is flushed
int trans_commit_barrier(void) {
    log_queue_node_t *node = (log_queue_node_t *)malloc(sizeof(log_queue_node_t));
    if (node == NULL) {
        return -1;
    }

    barrier_sync_t *barrier_sync = barrier_sync_init();
    if (barrier_sync == NULL) {
        free(node);
        return -1;
    }

    node->data = NULL;
    node->len = 0;
    node->is_barrier = 1;
    node->barrier_sync = barrier_sync;
    node->next = NULL;

    pthread_mutex_lock(&log_mutex);
    if (log_tail == NULL) {
        log_head = log_tail = node;
    } else {
        log_tail->next = node;
        log_tail = node;
    }
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);

    // Wait for the barrier to be processed
    barrier_sync_wait(barrier_sync);
    barrier_sync_destroy(barrier_sync);

    return 0;
}

// Count the number of items in the log queue
int get_log_queue_length(void) {
    int count = 0;
    pthread_mutex_lock(&log_mutex);
    log_queue_node_t *current = log_head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    pthread_mutex_unlock(&log_mutex);
    return count;
}

// Clean up the transaction system
void trans_destroy(void) {
    pthread_mutex_lock(&log_mutex);
    log_worker_running = 0;
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);

    pthread_join(log_worker_thread, NULL);

    // Clean up any remaining items in the queue
    log_queue_node_t *current = log_head;
    while (current != NULL) {
        log_queue_node_t *next = current->next;
        if (current->data) {
            free(current->data);
        }
        free(current);
        current = next;
    }

    log_head = log_tail = NULL;
}
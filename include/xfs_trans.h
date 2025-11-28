#ifndef XFS_TRANS_H
#define XFS_TRANS_H

#include <pthread.h>
#include <semaphore.h>

// Initialize the transaction system and log flushing thread
int trans_init(void);

// Add a metadata change to the in-memory log queue
int trans_add_item(void* data, int len);

// Commit a transaction barrier - blocks until the log is flushed
int trans_commit_barrier(void);

// Get the status of the log queue (number of pending transactions)
int get_log_queue_length(void);

// Clean up the transaction system
void trans_destroy(void);

#endif // XFS_TRANS_H
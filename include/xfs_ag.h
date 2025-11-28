#ifndef XFS_AG_H
#define XFS_AG_H

#include <pthread.h>
#include <stdint.h>

#define NUM_AGS 10  // Number of allocation groups

// Initialize allocation groups
int ag_init_headers(void);

// Lock a specific allocation group
int ag_lock(int ag_id);

// Unlock a specific allocation group
int ag_unlock(int ag_id);

// Get the offset of a specific AG in the disk
uint64_t ag_get_offset(int ag_id);

// Write AG headers to disk
int ag_write_headers(void);

#endif // XFS_AG_H
#ifndef XFS_DISK_H
#define XFS_DISK_H

#include <stdint.h>
#include <stddef.h>

// Initialize the simulated disk with a given size
int disk_init(size_t size);

// Read data from the simulated disk
int disk_read(uint64_t offset, void* buf, size_t len);

// Write data to the simulated disk
int disk_write(uint64_t offset, void* buf, size_t len);

// Clean up the simulated disk
void disk_destroy(void);

#endif // XFS_DISK_H
#ifndef XFS_ALLOC_H
#define XFS_ALLOC_H

#include <stdint.h>
#include "xfs_types.h"

#define XFS_BLOCK_SIZE 4096

// Allocate contiguous blocks in a specific AG
uint64_t xfs_alloc_blocks(int ag_id, int count);

// Free allocated blocks in a specific AG
int xfs_free_blocks(int ag_id, uint64_t start_block, int count);

// Initialize the allocator for an AG
int xfs_ag_init_alloc(int ag_id);

#endif // XFS_ALLOC_H
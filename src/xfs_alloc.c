#include "../include/xfs_alloc.h"
#include "../include/xfs_ag.h"
#include "../include/xfs_trans.h"
#include "../include/xfs_disk.h"
#include <stdio.h>
#include <string.h>

// Allocate contiguous blocks in a specific AG
uint64_t xfs_alloc_blocks(int ag_id, int count) {
    // Lock the allocation group
    if (ag_lock(ag_id) != 0) {
        return 0; // Allocation failed
    }
    
    // Get the AG's free space information
    xfs_agf_t agf;
    uint64_t ag_offset = ag_get_offset(ag_id);
    
    // Read the current AGF from disk
    if (disk_read(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        ag_unlock(ag_id);
        return 0;
    }
    
    // Find contiguous free blocks
    uint64_t start_block = 0;
    int found = 0;
    
    // Simplified allocation: Find the first available contiguous space
    for (int i = 2; i < 2400 - count; i++) { // Skip first 2 blocks (AGF and AGI)
        int is_free = 1;
        
        // Check if 'count' number of blocks starting at 'i' are free
        for (int j = 0; j < count; j++) {
            if (agf.free_blocks[i+j] == 1) { // 1 = used, 0 = free
                is_free = 0;
                i = i + j; // Skip to the next potentially free block
                break;
            }
        }
        
        if (is_free) {
            start_block = i;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        ag_unlock(ag_id);
        return 0; // No suitable space found
    }
    
    // Mark blocks as used
    for (int i = 0; i < count; i++) {
        agf.free_blocks[start_block + i] = 1; // Mark as used
    }
    
    // Update AGF metadata
    agf.agf_freeblks -= count;
    
    // Calculate the longest free space (simplified)
    agf.agf_longest = agf.agf_freeblks > 0 ? agf.agf_freeblks : 0;
    
    // Write the updated AGF back to disk
    if (disk_write(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        // If write failed, we should try to revert changes but for simulation purposes, 
        // we'll just return failure
        ag_unlock(ag_id);
        return 0;
    }
    
    // Log this allocation operation
    trans_add_item(&agf, sizeof(xfs_agf_t));
    
    // Unlock the allocation group
    ag_unlock(ag_id);
    
    // Return the starting block number within this AG
    return start_block;
}

// Free allocated blocks in a specific AG
int xfs_free_blocks(int ag_id, uint64_t start_block, int count) {
    // Lock the allocation group
    if (ag_lock(ag_id) != 0) {
        return -1;
    }
    
    // Get the AG's free space information
    xfs_agf_t agf;
    uint64_t ag_offset = ag_get_offset(ag_id);
    
    // Read the current AGF from disk
    if (disk_read(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        ag_unlock(ag_id);
        return -1;
    }
    
    // Mark blocks as free
    for (int i = 0; i < count; i++) {
        if (start_block + i < 2400) {
            agf.free_blocks[start_block + i] = 0; // Mark as free
        }
    }
    
    // Update AGF metadata
    agf.agf_freeblks += count;
    
    // Calculate the longest free space (simplified)
    if (agf.agf_longest < count) {
        agf.agf_longest = count;
    }
    
    // Write the updated AGF back to disk
    if (disk_write(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        ag_unlock(ag_id);
        return -1;
    }
    
    // Log this operation
    trans_add_item(&agf, sizeof(xfs_agf_t));
    
    // Unlock the allocation group
    ag_unlock(ag_id);
    
    return 0; // Success
}

// Initialize the allocator for an AG - mark all blocks as free initially
int xfs_ag_init_alloc(int ag_id) {
    // Lock the allocation group
    if (ag_lock(ag_id) != 0) {
        return -1;
    }
    
    // Get the AG's free space information
    xfs_agf_t agf;
    uint64_t ag_offset = ag_get_offset(ag_id);
    
    // Read the current AGF from disk
    if (disk_read(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        ag_unlock(ag_id);
        return -1;
    }
    
    // Initialize all blocks as free (0 = free, 1 = used)
    // Reserve first 2 blocks for AGF and AGI
    for (int i = 0; i < 2; i++) {
        agf.free_blocks[i] = 1; // Mark as used (reserved)
    }
    for (int i = 2; i < 2400; i++) {
        agf.free_blocks[i] = 0; // Mark as free
    }
    
    // Update AGF metadata
    agf.agf_freeblks = 2400 - 2; // All blocks except reserved ones
    agf.agf_longest = agf.agf_freeblks;
    
    // Write the updated AGF back to disk
    if (disk_write(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        ag_unlock(ag_id);
        return -1;
    }
    
    // Unlock the allocation group
    ag_unlock(ag_id);
    
    return 0; // Success
}
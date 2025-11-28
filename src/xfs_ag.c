#include "../include/xfs_ag.h"
#include "../include/xfs_types.h"
#include "../include/xfs_disk.h"
#include <stdlib.h>
#include <stdio.h>

// Array of mutexes for each allocation group
static pthread_mutex_t ag_mutexes[NUM_AGS];

// Initialize allocation groups
int ag_init_headers(void) {
    // Initialize mutexes for each AG
    for (int i = 0; i < NUM_AGS; i++) {
        if (pthread_mutex_init(&ag_mutexes[i], NULL) != 0) {
            // Clean up already initialized mutexes
            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&ag_mutexes[j]);
            }
            return -1;
        }
    }
    
    return 0;
}

// Lock a specific allocation group
int ag_lock(int ag_id) {
    if (ag_id < 0 || ag_id >= NUM_AGS) {
        return -1;
    }
    
    return pthread_mutex_lock(&ag_mutexes[ag_id]);
}

// Unlock a specific allocation group
int ag_unlock(int ag_id) {
    if (ag_id < 0 || ag_id >= NUM_AGS) {
        return -1;
    }
    
    return pthread_mutex_unlock(&ag_mutexes[ag_id]);
}

// Get the offset of a specific AG in the disk
uint64_t ag_get_offset(int ag_id) {
    if (ag_id < 0 || ag_id >= NUM_AGS) {
        return 0;  // Invalid AG ID
    }
    
    // Assuming each AG starts at regular intervals (e.g., every 10MB)
    // For this simulation, let's say each AG is 10MB (10 * 1024 * 1024 bytes)
    return (uint64_t)ag_id * (10 * 1024 * 1024);
}

// Write AG headers to disk
int ag_write_headers(void) {
    xfs_sb_t sb;
    xfs_agf_t agf;
    xfs_agi_t agi;
    
    // Initialize superblock
    sb.sb_magicnum = 0x58465342;  // "XFSB" in hex
    sb.sb_blocksize = 4096;       // 4KB blocks
    sb.sb_dblocks = 100 * 1024 * 1024 / 4096;  // Assuming 100MB total size
    sb.sb_agcount = NUM_AGS;
    sb.sb_versionnum = 5;
    
    // Write superblock at offset 0
    if (disk_write(0, &sb, sizeof(xfs_sb_t)) != 0) {
        return -1;
    }
    
    // Write AG headers for each AG
    for (int i = 0; i < NUM_AGS; i++) {
        uint64_t ag_offset = ag_get_offset(i);
        
        // Initialize AGF
        agf.agf_magicnum = 0x58414746;  // "XAGF" in hex
        agf.agf_length = 10 * 1024 * 1024 / 4096;  // Size in blocks
        agf.agf_freeblks = 10 * 1024 * 1024 / 4096 - 2;  // Subtract AGF and AGI blocks
        agf.agf_longest = 10 * 1024 * 1024 / 4096 - 2;
        
        // Write AGF at AG start
        if (disk_write(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
            return -1;
        }
        
        // Initialize AGI
        agi.agi_magicnum = 0x58414749;  // "XAGI" in hex
        agi.agi_count = 0;              // Initially no inodes
        agi.agi_root = 0;               // Root block of inode btree
        agi.agi_freecount = 0;          // Initially no free inodes
        
        // Write AGI at AG start + 1 block
        if (disk_write(ag_offset + 4096, &agi, sizeof(xfs_agi_t)) != 0) {
            return -1;
        }
    }
    
    return 0;
}
#include "../include/xfs_io.h"
#include "../include/xfs_alloc.h"
#include "../include/xfs_trans.h"
#include "../include/xfs_disk.h"
#include "../include/xfs_ag.h"
#include "../include/xfs_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Helper function to find which extent contains a given logical block
static xfs_extent_t* find_extent_for_offset(xfs_inode_t *inode, uint64_t logical_block) {
    for (int i = 0; i < inode->extent_count; i++) {
        if (logical_block >= inode->extents[i].start_off && 
            logical_block < (inode->extents[i].start_off + inode->extents[i].block_count)) {
            return &inode->extents[i];
        }
    }
    return NULL; // No extent found for this logical block
}

// Helper function to add a new extent to the inode
static int add_extent_to_inode(xfs_inode_t *inode, uint64_t logical_start, uint64_t physical_start, uint64_t block_count) {
    if (inode->extent_count >= 16) { // Maximum number of extents reached
        return -1;
    }
    
    inode->extents[inode->extent_count].start_off = logical_start;
    inode->extents[inode->extent_count].start_block = physical_start;
    inode->extents[inode->extent_count].block_count = block_count;
    inode->extent_count++;
    
    return 0;
}

// Write data to a file (simulated)
int xfs_sim_write(xfs_inode_t *inode, void *buffer, size_t size, off_t offset) {
    if (!inode || !buffer || size == 0) {
        return -1;
    }
    
    // Calculate number of blocks needed
    uint64_t block_start = offset / XFS_BLOCK_SIZE;
    uint64_t block_end = (offset + size - 1) / XFS_BLOCK_SIZE;
    uint64_t num_blocks = block_end - block_start + 1;
    
    printf("[XFS Write] Requested to write %zu bytes at offset %ld (%lu blocks)\n", size, offset, num_blocks);
    
    // Check each required block to see if it has a physical mapping
    for (uint64_t i = 0; i < num_blocks; i++) {
        uint64_t logical_block = block_start + i;
        
        // Check if this logical block is already mapped
        xfs_extent_t *extent = find_extent_for_offset(inode, logical_block);
        if (extent == NULL) {
            // Need to allocate physical blocks for this region
            // For simplicity, we'll allocate one block at a time but could optimize to allocate more
            uint64_t ag_id = logical_block % NUM_AGS; // Distribute across AGs based on block number
            printf("[XFS Write] Allocating 1 block in AG %d for logical block %lu\n", ag_id, logical_block);
            
            uint64_t physical_block = xfs_alloc_blocks(ag_id, 1);
            if (physical_block == 0) {
                printf("[XFS Write] Failed to allocate block in AG %d\n", ag_id);
                return -1; // Allocation failed
            }
            
            // Add the new extent to the inode
            if (add_extent_to_inode(inode, logical_block, physical_block, 1) != 0) {
                // If can't add to inode, free the allocated block
                xfs_free_blocks(ag_id, physical_block, 1);
                return -1;
            }
            
            printf("[XFS Write] Allocated physical block %lu for logical block %lu\n", physical_block, logical_block);
        }
    }
    
    // At this point, we have all necessary blocks allocated
    // Commit a transaction barrier before writing actual data
    printf("[XFS Write] Barrier: Waiting for log flush...\n");
    if (trans_commit_barrier() != 0) {
        printf("[XFS Write] Failed to commit barrier\n");
        return -1;
    }
    printf("[XFS Write] Barrier: Log flushed. Proceeding with data write.\n");
    
    // Now perform the actual writes to disk
    size_t bytes_written = 0;
    while (bytes_written < size) {
        // Calculate the current logical block and offset within that block
        uint64_t current_logical_block = (offset + bytes_written) / XFS_BLOCK_SIZE;
        size_t offset_in_block = (offset + bytes_written) % XFS_BLOCK_SIZE;
        
        // Find the physical extent for this logical block
        xfs_extent_t *extent = find_extent_for_offset(inode, current_logical_block);
        if (extent == NULL) {
            printf("[XFS Write] Error: No extent found for logical block %lu during write\n", current_logical_block);
            return -1;
        }
        
        // Calculate the physical block for this logical block
        uint64_t logical_block_offset = current_logical_block - extent->start_off;
        uint64_t physical_block = extent->start_block + logical_block_offset;
        
        // Calculate how much data to write in this block (either remaining in block or remaining in buffer)
        size_t bytes_to_write_in_block = XFS_BLOCK_SIZE - offset_in_block;
        if (bytes_to_write_in_block > (size - bytes_written)) {
            bytes_to_write_in_block = size - bytes_written;
        }
        
        // Calculate the disk offset for this physical block
        uint64_t disk_offset = physical_block * XFS_BLOCK_SIZE;
        
        // Write the data to the disk
        if (disk_write(disk_offset + offset_in_block, 
                      (char*)buffer + bytes_written, 
                      bytes_to_write_in_block) != 0) {
            printf("[XFS Write] Failed to write to disk at offset %lu\n", disk_offset + offset_in_block);
            return -1;
        }
        
        bytes_written += bytes_to_write_in_block;
    }
    
    // Update the file size if necessary
    off_t new_size = offset + size;
    if (new_size > inode->di_size) {
        inode->di_size = new_size;
    }
    
    printf("[XFS Write] Successfully wrote %zu bytes at offset %ld\n", size, offset);
    return bytes_written;
}

// Global inode storage for simulation
static xfs_inode_t inodes[100]; // Simulate storing up to 100 inodes
static char inode_names[100][64]; // Store names for up to 100 inodes (max 63 chars + null terminator)
static int max_inode_num = 0;
static int initialized = 0;

// Helper to initialize inodes
static void initialize_inodes(void) {
    if (!initialized) {
        for (int i = 0; i < 100; i++) {
            inodes[i].inode_num = 0;
            inodes[i].di_size = 0;
            inodes[i].extent_count = 0;
        }
        initialized = 1;
    }
}

// Format the disk (mkfs equivalent)
int xfs_mkfs(size_t disk_size) {
    // Initialize the disk
    if (disk_init(disk_size) != 0) {
        return -1;
    }

    // Initialize allocation groups
    if (ag_init_headers() != 0) {
        return -1;
    }

    // Format the disk (write AG headers)
    if (ag_write_headers() != 0) {
        return -1;
    }

    // Initialize the allocator for each AG
    for (int i = 0; i < NUM_AGS; i++) {
        if (xfs_ag_init_alloc(i) != 0) {
            return -1;
        }
    }

    return 0;
}

// Mount the filesystem
int xfs_mount(void) {
    // Initialize transaction system
    if (trans_init() != 0) {
        return -1;
    }

    return 0;
}

// Create a new file with a specific name (allocate an inode)
int xfs_create_named_file(const char* filename) {
    initialize_inodes();

    max_inode_num++;

    // Initialize the new inode
    inodes[max_inode_num].inode_num = max_inode_num;
    inodes[max_inode_num].di_mode = 0x1FF;  // -rw-rw-rw- permissions
    inodes[max_inode_num].di_uid = 1000;
    inodes[max_inode_num].di_gid = 1000;
    inodes[max_inode_num].di_nlink = 1;
    inodes[max_inode_num].di_size = 0;
    inodes[max_inode_num].extent_count = 0;

    // Copy the filename
    if (filename != NULL) {
        strncpy(inode_names[max_inode_num], filename, 63);
        inode_names[max_inode_num][63] = '\0'; // Ensure null termination
    } else {
        snprintf(inode_names[max_inode_num], 64, "unnamed_%d", max_inode_num);
    }

    printf("File '%s' created. Allocated Inode #%d\n", inode_names[max_inode_num], max_inode_num);
    return max_inode_num;
}

// Create a new file (allocate an inode) - creates with a default name
int xfs_create_file(void) {
    return xfs_create_named_file(NULL);
}

// Helper to get an inode by number
xfs_inode_t* get_inode_ptr(int inode_num) {
    initialize_inodes();

    if (inode_num <= 0 || inode_num > max_inode_num) {
        return NULL; // Invalid inode number
    }
    return &inodes[inode_num];
}

// Helper to get an inode by filename
xfs_inode_t* get_inode_by_name(const char* filename) {
    initialize_inodes();

    if (!filename) return NULL;

    for (int i = 1; i <= max_inode_num; i++) {
        if (inodes[i].inode_num > 0 && strcmp(inode_names[i], filename) == 0) {
            return &inodes[i];
        }
    }
    return NULL; // File not found
}

// Helper to get inode number by filename
int get_inode_num_by_name(const char* filename) {
    initialize_inodes();

    if (!filename) return -1;

    for (int i = 1; i <= max_inode_num; i++) {
        if (inodes[i].inode_num > 0 && strcmp(inode_names[i], filename) == 0) {
            return i;
        }
    }
    return -1; // File not found
}

// Print detailed inode information
void print_inode_details(int inode_num) {
    xfs_inode_t* node = get_inode_ptr(inode_num);
    if (!node) {
        printf("Error: Inode %d does not exist\n", inode_num);
        return;
    }

    printf("\n--- INODE %d METADATA ---\n", inode_num);
    printf("Size: %llu bytes\n", (unsigned long long)node->di_size);
    printf("Extents: %d\n", node->extent_count);
    for(int i = 0; i < node->extent_count; i++) {
        printf("  [%d] Logical: %llu -> PhysBlock: %llu (Len: %llu)\n",
               i,
               (unsigned long long)node->extents[i].start_off,
               (unsigned long long)node->extents[i].start_block,
               (unsigned long long)node->extents[i].block_count);
    }
    printf("--------------------------\n");
}

// Print log/journal queue status
void print_log_queue_status(void) {
    int queue_length = get_log_queue_length();
    printf("\n--- LOG/JOURNAL QUEUE STATUS ---\n");
    printf("Pending transactions in queue: %d\n", queue_length);
    printf("-------------------------------\n");
}

// Print superblock information
void print_superblock_info(void) {
    xfs_sb_t sb;

    // Read the superblock from disk
    if (disk_read(0, &sb, sizeof(xfs_sb_t)) != 0) {
        printf("Error reading superblock from disk.\n");
        return;
    }

    printf("\n--- SUPERBLOCK METADATA ---\n");
    printf("Magic Number: 0x%X\n", sb.sb_magicnum);
    printf("Block Size: %u bytes\n", sb.sb_blocksize);
    printf("Total Data Blocks: %llu\n", (unsigned long long)sb.sb_dblocks);
    printf("Number of AGs: %llu\n", (unsigned long long)sb.sb_agcount);
    printf("Version: %u\n", sb.sb_versionnum);
    printf("--------------------------\n");
}

// Print AGF (Allocation Group Free Space) information
void print_agf_info(int ag_id) {
    if (ag_id < 0 || ag_id >= NUM_AGS) {
        printf("Invalid AG ID: %d\n", ag_id);
        return;
    }

    xfs_agf_t agf;
    uint64_t ag_offset = ag_get_offset(ag_id);

    // Read the AGF from disk
    if (disk_read(ag_offset, &agf, sizeof(xfs_agf_t)) != 0) {
        printf("Error reading AGF for AG %d from disk.\n", ag_id);
        return;
    }

    printf("\n--- AGF (AG %d) METADATA ---\n", ag_id);
    printf("Magic Number: 0x%X\n", agf.agf_magicnum);
    printf("AG Length: %u blocks\n", agf.agf_length);
    printf("Free Blocks: %u\n", agf.agf_freeblks);
    printf("Longest Free Space: %u blocks\n", agf.agf_longest);

    // Count number of used vs free blocks
    int used_blocks = 0;
    int free_blocks = 0;
    for (int i = 0; i < 2400; i++) {
        if (agf.free_blocks[i] == 1) {
            used_blocks++;
        } else if (agf.free_blocks[i] == 0) {
            free_blocks++;
        }
    }
    printf("Blocks in use: %d\n", used_blocks);
    printf("Blocks free: %d\n", free_blocks);
    printf("--------------------------\n");
}

// Print AGI (Allocation Group Inode) information
void print_agi_info(int ag_id) {
    if (ag_id < 0 || ag_id >= NUM_AGS) {
        printf("Invalid AG ID: %d\n", ag_id);
        return;
    }

    xfs_agi_t agi;
    uint64_t ag_offset = ag_get_offset(ag_id) + 4096; // AGI is at offset 1 block from start of AG

    // Read the AGI from disk
    if (disk_read(ag_offset, &agi, sizeof(xfs_agi_t)) != 0) {
        printf("Error reading AGI for AG %d from disk.\n", ag_id);
        return;
    }

    printf("\n--- AGI (AG %d) METADATA ---\n", ag_id);
    printf("Magic Number: 0x%X\n", agi.agi_magicnum);
    printf("Total Inodes: %u\n", agi.agi_count);
    printf("Root of Inode Btree: %u\n", agi.agi_root);
    printf("Free Inodes: %u\n", agi.agi_freecount);
    printf("--------------------------\n");
}

// Print summary of all AGs
void print_ag_summary(void) {
    printf("\n--- ALLOCATION GROUP SUMMARY ---\n");
    for (int i = 0; i < NUM_AGS; i++) {
        xfs_agf_t agf;
        uint64_t ag_offset = ag_get_offset(i);

        if (disk_read(ag_offset, &agf, sizeof(xfs_agf_t)) == 0) {
            printf("AG %d: %u free blocks of %u total\n", i, agf.agf_freeblks, agf.agf_length);
        }
    }
    printf("--------------------------------\n");
}

// List all files (inodes) in the system
void list_files(void) {
    if (!initialized) {
        printf("No files have been created yet. Inode system not initialized.\n");
        return;
    }

    if (max_inode_num <= 0) {
        printf("No files exist in the system.\n");
        return;
    }

    printf("\n--- FILE LIST ---\n");
    printf("ID\tSize\tExtents\tName\n");
    printf("--\t----\t-------\t----\n");

    for (int i = 1; i <= max_inode_num; i++) {
        if (inodes[i].inode_num > 0) {  // Check if inode exists
            printf("%d\t%llu\t%d\t%s\n",
                   inodes[i].inode_num,
                   (unsigned long long)inodes[i].di_size,
                   inodes[i].extent_count,
                   inode_names[i]);
        }
    }
    printf("-----------------\n");
}

// Read data from a file (simulated)
int xfs_sim_read(xfs_inode_t *inode, void *buffer, size_t size, off_t offset) {
    if (!inode || !buffer || size == 0) {
        return -1;
    }
    
    // Check if the read would go beyond the file size
    if (offset >= inode->di_size) {
        return 0; // At or beyond end of file
    }
    
    // Limit read to the actual file size
    size_t size_to_read = size;
    if (offset + size_to_read > inode->di_size) {
        size_to_read = inode->di_size - offset;
    }
    
    printf("[XFS Read] Requested to read %zu bytes at offset %ld\n", size_to_read, offset);
    
    // Perform the actual reads from disk
    size_t bytes_read = 0;
    while (bytes_read < size_to_read) {
        // Calculate the current logical block and offset within that block
        uint64_t current_logical_block = (offset + bytes_read) / XFS_BLOCK_SIZE;
        size_t offset_in_block = (offset + bytes_read) % XFS_BLOCK_SIZE;
        
        // Find the physical extent for this logical block
        xfs_extent_t *extent = find_extent_for_offset(inode, current_logical_block);
        if (extent == NULL) {
            // No extent for this block - it's a "hole" in the file, return zeros
            size_t bytes_to_zero = XFS_BLOCK_SIZE - offset_in_block;
            if (bytes_to_zero > (size_to_read - bytes_read)) {
                bytes_to_zero = size_to_read - bytes_read;
            }
            
            // Zero out the buffer for this "hole"
            memset((char*)buffer + bytes_read, 0, bytes_to_zero);
            bytes_read += bytes_to_zero;
            continue;
        }
        
        // Calculate the physical block for this logical block
        uint64_t logical_block_offset = current_logical_block - extent->start_off;
        uint64_t physical_block = extent->start_block + logical_block_offset;
        
        // Calculate how much data to read in this block (either remaining in block or remaining in buffer)
        size_t bytes_to_read_in_block = XFS_BLOCK_SIZE - offset_in_block;
        if (bytes_to_read_in_block > (size_to_read - bytes_read)) {
            bytes_to_read_in_block = size_to_read - bytes_read;
        }
        
        // Calculate the disk offset for this physical block
        uint64_t disk_offset = physical_block * XFS_BLOCK_SIZE;
        
        // Read the data from the disk
        if (disk_read(disk_offset + offset_in_block, 
                     (char*)buffer + bytes_read, 
                     bytes_to_read_in_block) != 0) {
            printf("[XFS Read] Failed to read from disk at offset %lu\n", disk_offset + offset_in_block);
            return -1;
        }
        
        bytes_read += bytes_to_read_in_block;
    }
    
    printf("[XFS Read] Successfully read %zu bytes at offset %ld\n", bytes_read, offset);
    return bytes_read;
}
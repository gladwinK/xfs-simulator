#ifndef XFS_TYPES_H
#define XFS_TYPES_H

#include <stdint.h>
#include <stddef.h>

// XFS Superblock
typedef struct {
    uint32_t sb_magicnum;    // Magic number
    uint32_t sb_blocksize;   // File system block size
    uint64_t sb_dblocks;     // Number of data blocks
    uint64_t sb_agcount;     // Number of allocation groups
    uint32_t sb_versionnum;  // Header version
} xfs_sb_t;

// XFS AG Free Space
typedef struct {
    uint32_t agf_magicnum;   // Magic number
    uint32_t agf_length;     // Total length in blocks
    uint32_t agf_freeblks;   // Total free blocks
    uint32_t agf_longest;    // Longest free space
    // Simple free space tracking - in a real implementation this would be more complex
    uint8_t free_blocks[2400]; // Array to track free/used blocks (assuming 10MB AG with 4KB blocks = 2560 blocks, but leaving some room)
} xfs_agf_t;

// XFS AG Inode
typedef struct {
    uint32_t agi_magicnum;   // Magic number
    uint32_t agi_count;      // Number of inodes
    uint32_t agi_root;       // Root of inode btree
    uint32_t agi_freecount;  // Number of free inodes
} xfs_agi_t;

// Represents a mapping: "Logical Offset 0 maps to Physical Block 100 for 10 blocks"
typedef struct {
    uint64_t start_off;   // Logical file offset (in blocks)
    uint64_t start_block; // Physical starting block on disk
    uint64_t block_count; // Number of contiguous blocks
} xfs_extent_t;

// XFS Inode
typedef struct {
    uint32_t inode_num;
    uint16_t di_mode;        // File mode
    uint32_t di_uid;         // User ID
    uint32_t di_gid;         // Group ID
    uint32_t di_nlink;       // Link count
    uint64_t di_size;        // File size in bytes

    // In real XFS, this is a B+ Tree.
    // For simulation, use a fixed array or linked list.
    int extent_count;
    xfs_extent_t extents[16]; // Hard limit of 16 extents for simplicity
} xfs_inode_t;

// XFS Transaction
typedef struct {
    void *data;              // Pointer to transaction data
    size_t len;              // Length of data
    int type;                // Type of transaction item
} xfs_trans_t;

#endif // XFS_TYPES_H
#ifndef XFS_IO_H
#define XFS_IO_H

#include "xfs_types.h"
#include <sys/types.h>  // For off_t

// Read data from a file (simulated)
int xfs_sim_read(xfs_inode_t *inode, void *buffer, size_t size, off_t offset);

// Write data to a file (simulated)
int xfs_sim_write(xfs_inode_t *inode, void *buffer, size_t size, off_t offset);

// Print detailed inode information
void print_inode_details(int inode_num);

// Helper to get an inode by number
xfs_inode_t* get_inode_ptr(int inode_num);

// Helper to get an inode by filename
xfs_inode_t* get_inode_by_name(const char* filename);

// Helper to get inode number by filename
int get_inode_num_by_name(const char* filename);

// Create a new file (allocate an inode)
int xfs_create_file(void);

// Create a new file with a specific name
int xfs_create_named_file(const char* filename);

// Print log/journal queue status
void print_log_queue_status(void);

// Format disk (mkfs equivalent)
int xfs_mkfs(size_t disk_size);

// Mount filesystem
int xfs_mount(void);

// Print superblock information
void print_superblock_info(void);

// Print AGF (Allocation Group Free Space) information
void print_agf_info(int ag_id);

// Print AGI (Allocation Group Inode) information
void print_agi_info(int ag_id);

// Print summary of all AGs
void print_ag_summary(void);

// List all files in the system
void list_files(void);

#endif // XFS_IO_H
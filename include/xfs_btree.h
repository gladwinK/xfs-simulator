#ifndef XFS_BTREE_H
#define XFS_BTREE_H

#include <stdint.h>

// A simplified B+ tree node structure
typedef struct xfs_btree_node {
    int is_leaf;  // 1 if leaf, 0 if internal
    int num_keys;
    uint64_t keys[10];  // Array of keys (simplified)
    void* values[10];   // Array of values (simplified)
    struct xfs_btree_node* next;  // Next node pointer for linked list implementation
} xfs_btree_node_t;

// Initialize a B+ tree
xfs_btree_node_t* btree_init(void);

// Insert a key-value pair into the B+ tree
int btree_insert(xfs_btree_node_t* root, uint64_t key, void* value);

// Look up a value by key in the B+ tree
void* btree_lookup(xfs_btree_node_t* root, uint64_t key);

// Destroy the B+ tree
void btree_destroy(xfs_btree_node_t* root);

#endif // XFS_BTREE_H
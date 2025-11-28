#include "../include/xfs_btree.h"
#include <stdlib.h>
#include <string.h>

// Initialize a B+ tree
xfs_btree_node_t* btree_init(void) {
    xfs_btree_node_t* root = (xfs_btree_node_t*)malloc(sizeof(xfs_btree_node_t));
    if (root == NULL) {
        return NULL;
    }
    
    root->is_leaf = 1;
    root->num_keys = 0;
    for (int i = 0; i < 10; i++) {
        root->keys[i] = 0;
        root->values[i] = NULL;
    }
    root->next = NULL;
    
    return root;
}

// Insert a key-value pair into the B+ tree
int btree_insert(xfs_btree_node_t* root, uint64_t key, void* value) {
    // In this simplified implementation, we're using a linear approach
    // to insert into our "B+ tree" which is actually just a linked list of nodes
    
    if (root == NULL) {
        return -1;
    }
    
    // Find the correct position in the leaf level
    xfs_btree_node_t* current = root;
    while (current != NULL) {
        // If we have space in the current node
        if (current->num_keys < 10) {
            int insert_pos = 0;
            
            // Find the correct position to maintain sorted order
            while (insert_pos < current->num_keys && current->keys[insert_pos] < key) {
                insert_pos++;
            }
            
            // Shift elements to the right to make space
            for (int i = current->num_keys; i > insert_pos; i--) {
                current->keys[i] = current->keys[i-1];
                current->values[i] = current->values[i-1];
            }
            
            // Insert the new key-value pair
            current->keys[insert_pos] = key;
            current->values[insert_pos] = value;
            current->num_keys++;
            
            return 0;
        }
        
        // If no space in this node, go to the next node
        if (current->next == NULL) {
            // Create a new node if we reach the end
            current->next = btree_init();
            if (current->next == NULL) {
                return -1;
            }
            current = current->next;
        } else {
            current = current->next;
        }
    }
    
    return 0;
}

// Look up a value by key in the B+ tree
void* btree_lookup(xfs_btree_node_t* root, uint64_t key) {
    if (root == NULL) {
        return NULL;
    }
    
    xfs_btree_node_t* current = root;
    while (current != NULL) {
        // Search in this node
        for (int i = 0; i < current->num_keys; i++) {
            if (current->keys[i] == key) {
                return current->values[i];
            } else if (current->keys[i] > key) {
                // If the key is greater than what we're looking for, 
                // it's not in this node or subsequent nodes (since it's sorted)
                return NULL;
            }
        }
        
        // Move to the next node
        current = current->next;
    }
    
    return NULL;
}

// Destroy the B+ tree
void btree_destroy(xfs_btree_node_t* root) {
    xfs_btree_node_t* current = root;
    while (current != NULL) {
        xfs_btree_node_t* next = current->next;
        free(current);
        current = next;
    }
}
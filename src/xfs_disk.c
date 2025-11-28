#include "../include/xfs_disk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint8_t *DISK_MEMORY = NULL;
static size_t DISK_SIZE = 0;

int disk_init(size_t size) {
    if (DISK_MEMORY != NULL) {
        free(DISK_MEMORY);
    }
    
    DISK_MEMORY = (uint8_t *)malloc(size);
    if (DISK_MEMORY == NULL) {
        return -1;
    }
    
    DISK_SIZE = size;
    memset(DISK_MEMORY, 0, size);
    
    return 0;
}

int disk_read(uint64_t offset, void* buf, size_t len) {
    if (DISK_MEMORY == NULL) {
        return -1;
    }
    
    if (offset + len > DISK_SIZE) {
        return -1;  // Out of bounds
    }
    
    memcpy(buf, DISK_MEMORY + offset, len);
    return 0;
}

int disk_write(uint64_t offset, void* buf, size_t len) {
    if (DISK_MEMORY == NULL) {
        return -1;
    }
    
    if (offset + len > DISK_SIZE) {
        return -1;  // Out of bounds
    }
    
    memcpy(DISK_MEMORY + offset, buf, len);
    return 0;
}

void disk_destroy(void) {
    if (DISK_MEMORY != NULL) {
        free(DISK_MEMORY);
        DISK_MEMORY = NULL;
        DISK_SIZE = 0;
    }
}
#include "../include/xfs_disk.h"
#include "../include/xfs_trans.h"
#include "../include/xfs_ag.h"
#include "../include/xfs_types.h"
#include "../include/xfs_alloc.h"
#include "../include/xfs_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// Helper to print metadata state
void inspect_inode(int inode_num) {
    print_inode_details(inode_num);
}

int main() {
    char input[256];
    char *cmd;
    
    printf("XFS Simulation Shell. Type 'help' for commands.\n");

    while (1) {
        printf("XFS_SIM> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        // Remove newline
        input[strcspn(input, "\n")] = 0;

        // Split command
        cmd = strtok(input, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "help") == 0) {
            printf("Available commands:\n");
            printf("  format          - Format the disk (mkfs equivalent)\n");
            printf("  mount           - Mount the filesystem\n");
            printf("  create          - Create a new file and allocate an inode\n");
            printf("  write <inode> <data> - Write data to an inode\n");
            printf("  read <inode>    - Read data from an inode\n");
            printf("  inspect <inode> - Show detailed inode metadata\n");
            printf("  ls/list         - List all files in the system\n");
            printf("  superblock      - Show superblock information\n");
            printf("  agf <ag_id>     - Show AG Free Space (AGF) information\n");
            printf("  agi <ag_id>     - Show AG Inode (AGI) information\n");
            printf("  ag_summary      - Show summary of all allocation groups\n");
            printf("  log             - Show transaction log status\n");
            printf("  barrier_test    - Test the barrier mechanism\n");
            printf("  exit            - Exit the simulator\n");

        } else if (strcmp(cmd, "format") == 0) {
            printf("Formatting disk...\n");
            // Call your format function
            if (xfs_mkfs(100 * 1024 * 1024) == 0) {
                printf("Disk formatted. Superblock created.\n");
            } else {
                printf("Failed to format disk.\n");
            }
        
        } else if (strcmp(cmd, "mount") == 0) {
            if (xfs_mount() == 0) {
                printf("Filesystem mounted. AGs initialized.\n");
            } else {
                printf("Failed to mount filesystem.\n");
            }

        } else if (strcmp(cmd, "create") == 0) {
            char *filename = strtok(NULL, " ");
            int inode_num;
            if (filename) {
                inode_num = xfs_create_named_file(filename);
            } else {
                inode_num = xfs_create_file(); // Create with default name
            }
            inspect_inode(inode_num); // SHOW METADATA IMMEDIATELY

        } else if (strcmp(cmd, "write") == 0) {
            // Usage: write <filename> <data> OR write <inode_num> <data>
            char *arg1 = strtok(NULL, " ");
            char *data = strtok(NULL, "");

            if (arg1 && data) {
                // Try to parse as inode number first
                char *endptr;
                long inode_num = strtol(arg1, &endptr, 10);

                xfs_inode_t *node = NULL;
                int target_inode_num = -1;

                if (*endptr == '\0') {
                    // It's a number (inode number)
                    target_inode_num = (int)inode_num;
                    node = get_inode_ptr(target_inode_num);
                } else {
                    // It's a filename
                    target_inode_num = get_inode_num_by_name(arg1);
                    if (target_inode_num != -1) {
                        node = get_inode_ptr(target_inode_num);
                    }
                }

                if (node) {
                    printf("Writing '%s' to file (Inode %d)...\n", data, target_inode_num);
                    xfs_sim_write(node, data, strlen(data), 0);

                    printf("Write complete.\n");
                    inspect_inode(target_inode_num); // SHOW CHANGE IN EXTENTS
                } else {
                    printf("Error: File '%s' does not exist\n", arg1);
                }
            } else {
                printf("Usage: write <filename> <string_data> or write <inode_num> <string_data>\n");
            }

        } else if (strcmp(cmd, "read") == 0) {
            // Usage: read <filename> OR read <inode_num>
            char *arg1 = strtok(NULL, " ");
            if (arg1) {
                // Try to parse as inode number first
                char *endptr;
                long inode_num = strtol(arg1, &endptr, 10);

                xfs_inode_t *node = NULL;
                int target_inode_num = -1;

                if (*endptr == '\0') {
                    // It's a number (inode number)
                    target_inode_num = (int)inode_num;
                    node = get_inode_ptr(target_inode_num);
                } else {
                    // It's a filename
                    target_inode_num = get_inode_num_by_name(arg1);
                    if (target_inode_num != -1) {
                        node = get_inode_ptr(target_inode_num);
                    }
                }

                if (node) {
                    char buffer[1024] = {0};
                    int bytes_read = xfs_sim_read(node, buffer, 1023, 0);
                    if (bytes_read >= 0) {
                        buffer[bytes_read] = '\0'; // Ensure null termination
                        printf("READ OUTPUT: %s\n", buffer);
                    } else {
                        printf("Read operation failed\n");
                    }
                } else {
                    printf("Error: File '%s' does not exist\n", arg1);
                }
            } else {
                printf("Usage: read <filename> or read <inode_num>\n");
            }

        } else if (strcmp(cmd, "inspect") == 0) {
            // Usage: inspect <filename> OR inspect <inode>
            char *arg1 = strtok(NULL, " ");
            if (arg1) {
                // Try to parse as inode number first
                char *endptr;
                long inode_num = strtol(arg1, &endptr, 10);

                int target_inode_num = -1;

                if (*endptr == '\0') {
                    // It's a number (inode number)
                    target_inode_num = (int)inode_num;
                } else {
                    // It's a filename
                    target_inode_num = get_inode_num_by_name(arg1);
                }

                if (target_inode_num != -1) {
                    inspect_inode(target_inode_num);
                } else {
                    printf("Error: File '%s' does not exist\n", arg1);
                }
            } else {
                printf("Usage: inspect <filename> or inspect <inode_num>\n");
            }

        } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "list") == 0) {
            // List all files in the system
            list_files();

        } else if (strcmp(cmd, "log") == 0) {
            // Print the current state of the transaction log
            print_log_queue_status();

        } else if (strcmp(cmd, "superblock") == 0) {
            // Print superblock information
            print_superblock_info();

        } else if (strcmp(cmd, "agf") == 0) {
            // Print AGF information for a specific AG
            char *arg1 = strtok(NULL, " ");
            if (arg1) {
                int ag_id = atoi(arg1);
                print_agf_info(ag_id);
            } else {
                printf("Usage: agf <ag_id>\n");
            }

        } else if (strcmp(cmd, "agi") == 0) {
            // Print AGI information for a specific AG
            char *arg1 = strtok(NULL, " ");
            if (arg1) {
                int ag_id = atoi(arg1);
                print_agi_info(ag_id);
            } else {
                printf("Usage: agi <ag_id>\n");
            }

        } else if (strcmp(cmd, "ag_summary") == 0) {
            // Print summary of all AGs
            print_ag_summary();

        } else if (strcmp(cmd, "barrier_test") == 0) {
            printf("[CMD] Initiating Barrier Test...\n");
            int queue_before = get_log_queue_length();
            printf("[LOG] Log Queue has %d pending items before barrier.\n", queue_before);
            
            printf("[BARRIER] Thread waiting...\n");
            if (trans_commit_barrier() == 0) {
                int queue_after = get_log_queue_length();
                printf("[BARRIER] Barrier complete. Log Queue now has %d items.\n", queue_after);
            } else {
                printf("[BARRIER] Barrier failed.\n");
            }

        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
    
    // Clean up
    trans_destroy();
    disk_destroy();
    
    return 0;
}
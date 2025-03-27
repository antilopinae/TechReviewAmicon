// journal_utility.c
#include <stdio.h>      // Для fprintf, perror, fopen, fclose, fwrite
#include <stdlib.h>     // Для exit, EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // Для getopt
#include <string.h>     // Для strerror
#include <errno.h>      // Для errno

#include "shared_memory.h" // Include our shared memory API header

// ********************************* //

// Функция обратного вызова для чтения из разделяемой памяти и записи в файл.
static shared_memory_status_t write_to_file_action(shared_memory_t *shm, void *args) {
    FILE *file = (FILE *)args;
    if (!shm || !file) {
        fprintf(stderr, "write_to_file_action: Null argument(s)\n");
        return SHARED_MEMORY_STATUS_ERROR_NULL_POINTER;
    }

    // Assuming we want to write the entire shared memory region to file.
    // Adjust size and offset as needed if you have a specific data structure within SHM.
    size_t bytes_written = fwrite(shm->memory_region, 1, shm->size, file);
    if (bytes_written != shm->size) {
        perror("fwrite failed in write_to_file_action");
        return SHARED_MEMORY_STATUS_ERROR_SHM_OPEN; // Reusing SHM_OPEN error status for file write error
    }

    return SHARED_MEMORY_STATUS_SUCCESS;
}

// ********************************* //

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_file_path = argv[1];
    FILE *output_file = NULL;
    shared_memory_t *shm = NULL;
    shared_memory_status_t shm_status;

    // 1. Open the output file for writing
    output_file = fopen(output_file_path, "wb"); // "wb" for binary write, more robust for shared memory data
    if (!output_file) {
        perror("fopen failed");
        return EXIT_FAILURE;
    }

    // 2. Open the shared memory
    shm = shared_memory_open();
    if (!shm) {
        fprintf(stderr, "Failed to open shared memory: %d\n", SHARED_MEMORY_STATUS_ERROR_SHM_OPEN);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    // 3. Read from shared memory and write to file using callback
    shm_status = shared_memory_read(shm, write_to_file_action, output_file);
    if (shm_status != SHARED_MEMORY_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to read shared memory and write to file: %d\n", shm_status);
        fclose(output_file);
        shared_memory_close(shm);
        return EXIT_FAILURE;
    }

    // 4. Close the output file
    if (fclose(output_file) != 0) {
        perror("fclose failed");
        shared_memory_close(shm); // Still attempt to close shared memory even if file close fails
        return EXIT_FAILURE;
    }

    printf("Shared memory content saved to '%s'\n", output_file_path);

    // 5. Close the shared memory
    shm_status = shared_memory_close(shm);
    if (shm_status != SHARED_MEMORY_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to close shared memory: %d\n", shm_status);
        return EXIT_FAILURE; // Or consider continuing as data is already saved, but better to report error
    }

    return EXIT_SUCCESS;
}

// ********************************* //
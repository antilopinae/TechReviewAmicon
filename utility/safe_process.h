#ifndef SAFE_PROCESS_H
#define SAFE_PROCESS_H

#include <sys/types.h>

typedef enum safe_process_result {
    SAFE_PROCESS_OK = 0,
    SAFE_PROCESS_ERROR
} safe_process_result_t;

typedef struct safe_process{
    pid_t pid;
    void (*clear_process_before_death)(void*);
} safe_process_t;

// Fill struct and create child process
safe_process_t safe_process_create(void (*child_job)(void* args, safe_process_t my_process), void* args, void (*clear_process_before_death)(void*));

// Clear resources and exit process
void safe_process_delete_this(safe_process_t process, void* args);

// Check if parent is death then delete child (this process)
safe_process_result_t safe_process_check_status(safe_process_t process, void *args_to_delete);

#endif

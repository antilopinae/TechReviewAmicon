#include "safe_process.h"

#include <stdlib.h>
#include <unistd.h>

#include "utility.h"

safe_process_t safe_process_create(void (*child_job)(void* args, safe_process_t my_process), void* args, void (*clear_process_before_death)(void*))
{
    safe_process_t sp;
    sp.clear_process_before_death = clear_process_before_death;

    sp.pid = fork();

    if (sp.pid < 0) {
        DEBUG_LOG("Error fork: safe_process_create");
        exit(EXIT_FAILURE);
    } else if (sp.pid == 0) {
        child_job(args, sp);
    }

    return sp;
}

void safe_process_delete_this(safe_process_t sp, void* args)
{
    sp.clear_process_before_death(args);
    exit(EXIT_SUCCESS);
}

safe_process_result_t safe_process_check_status(safe_process_t sp, void *args_to_delete)
{
    if (getppid() == 1) {
        safe_process_delete_this(sp, args_to_delete);
        return SAFE_PROCESS_ERROR;
    }
    return SAFE_PROCESS_OK;
}

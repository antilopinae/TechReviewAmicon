#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>

#include "config.h"
#include "journal_transfer.h"

// "Usage: %s <socket_path> <file_path>\n"
int main(int argc, char* argv[])
{
    const char* socket_path = SERVER_UNIX_SOCKET_PATH;
    const char* file_path = JOURNAL_FILE_PATH;

    if (journal_transfer_rcv_and_write_file(socket_path, file_path) != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
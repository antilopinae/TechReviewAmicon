#ifndef JOURNAL_TRANSFER_H
#define JOURNAL_TRANSFER_H

#include "journal.h"

// Running receiver for journal transfer (blocking operation)
int journal_transfer_run_receiver(const char* socket_path, journal_t* journal);

// Send message to receiver to get journal and then write it to file
int journal_transfer_rcv_and_write_file(const char* socket_path, const char* file_path);

#endif    // JOURNAL_TRANSFER_H
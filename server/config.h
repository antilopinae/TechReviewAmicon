#ifndef CONFIG_H
#define CONFIG_H

#define JOURNAL_MAX_SIZE 5 * 1024 * 1024    // needed for easy journal receiver in unix-socket connection

// Server Configuration
#define SERVER_NUM_WORKERS 5
#define SERVER_ADDR "127.0.0.1"
#define SERVER_BASE_PORT 5000
#define SERVER_MAX_JOURNAL_SIZE 5 * 1024 * 1024
#define SERVER_UNIX_SOCKET_PATH "/tmp/server_unix_socket"

// Journal Utility
#define JOURNAL_FILE_PATH "note.txt"

#endif    // CONFIG_H
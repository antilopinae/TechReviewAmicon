#include "journal_transfer.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "config.h"
#include "utility.h"

int journal_transfer_run_receiver(const char* socket_path, journal_t* journal)
{
    int sockfd, client_sockfd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        DEBUG_LOG("Error: creation socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);
    unlink(socket_path);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        DEBUG_LOG("Error: unix socket bind");
        return -1;
    }

    if (listen(sockfd, 1) == -1)
    {
        DEBUG_LOG("Error: unix socket listen");
        close(sockfd);
        return -1;
    }

    client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd == -1)
    {
        DEBUG_LOG("Error: unix socket accept");
        close(sockfd);
        return -1;
    }
    close(sockfd);

    printf("Journal transfer started (PID %d)\n", getpid());

    size_t journal_length = SERVER_MAX_JOURNAL_SIZE;

    void* journal_content = malloc(SERVER_MAX_JOURNAL_SIZE);
    if (!journal_content)
    {
        DEBUG_LOG("Error: journal content malloc");
        close(client_sockfd);
        return -1;
    }

    journal_status_t read_status = journal_read(journal, journal_content, &journal_length);
    if (read_status != JOURNAL_STATUS_SUCCESS)
    {
        DEBUG_LOG("Journal_read failed with status: %d\n", read_status);
        close(client_sockfd);
        SAFE_FREE(journal_content);
        return -1;
    }

    ssize_t bytes_sent = send(client_sockfd, journal_content, journal_length, 0);
    if (bytes_sent == -1)
    {
        DEBUG_LOG("Error: unix socket send");
        close(client_sockfd);
        SAFE_FREE(journal_content);
        return -1;
    }
    // TODO: continue sending
    if ((size_t)bytes_sent != journal_length)
    {
        DEBUG_LOG("Sent incomplete journal content: sent %zd, expected %zu\n", bytes_sent, journal_length);
    }
    else
    {
        DEBUG_LOG("Parent sent journal content: %zd bytes\n", bytes_sent);
    }

    close(client_sockfd);
    SAFE_FREE(journal_content);

    return 0;
}

int journal_transfer_rcv_and_write_file(const char* socket_path, const char* file_path)
{
    int sockfd;
    struct sockaddr_un server_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        DEBUG_LOG("Error: unix socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        DEBUG_LOG("Error: unix socket connect");
        close(sockfd);
        return -1;
    }

    const char* trigger_message = "GET_JOURNAL";
    if (send(sockfd, trigger_message, strlen(trigger_message), 0) == -1)
    {
        DEBUG_LOG("Error: unix socket send trigger");
        close(sockfd);
        return -1;
    }

    FILE* output_file = fopen(file_path, "wb");
    if (!output_file)
    {
        DEBUG_LOG("Error: file open");
        close(sockfd);
        return -1;
    }

    char buffer[JOURNAL_MAX_SIZE];
    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
    {
        if (fwrite(buffer, 1, bytes_received, output_file) != (size_t)bytes_received)
        {
            DEBUG_LOG("Error: receive journal failed\n");
            fclose(output_file);
            close(sockfd);
            return -1;
        }
        total_bytes_received += bytes_received;
    }

    if (total_bytes_received == 0)
    {
        DEBUG_LOG("Error: receive journal failed\n");
        fclose(output_file);
        close(sockfd);
        return -1;
    }

    printf("Process received journal content: %zu bytes, saved to file: %s\n", total_bytes_received, file_path);

    fclose(output_file);
    close(sockfd);
    return 0;
}

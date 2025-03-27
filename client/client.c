#include <stdio.h>      // For fprintf, perror, printf, fgets, stdin, stderr
#include <stdlib.h>     // For EXIT_SUCCESS, EXIT_FAILURE, malloc, free
#include <string.h>     // For strlen, memset, strcpy
#include <arpa/inet.h>  // For inet_addr, htons

#include "client_worker.h" // Include the client_worker API header
#include "message.h"       // Include message API header

// ********************************* //

#define SERVER_IP_ADDRESS "127.0.0.1" // Loopback for local testing
#define SERVER_PORT 2000            // Port number of the server

// ********************************* //

int main() {
    client_worker_t *worker = NULL;
    client_worker_status_t client_status;
    message_t *message_to_send = NULL;
    message_t *received_message = NULL;
    char input_buffer[2000]; // Buffer for user input
    char *datetime_str = NULL;

    // 1. Create ClientWorker instance
    worker = client_worker_create();
    if (!worker) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Failed to create client worker\n", datetime_str ? datetime_str : "ERROR");
        free(datetime_str);
        return EXIT_FAILURE;
    }

    // 2. Connect to the server
    client_status = client_worker_connect(worker, inet_addr(SERVER_IP_ADDRESS), htons(SERVER_PORT));
    if (client_status != CLIENT_WORKER_STATUS_SUCCESS) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Failed to connect to server: %d\n", datetime_str ? datetime_str : "ERROR", client_status);
        free(datetime_str);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }
    printf("Connected to server at %s:%d\n", SERVER_IP_ADDRESS, SERVER_PORT);

    // 3. Get user input message
    printf("Enter message to send to server: ");
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
        datetime_str = get_current_datetime_string();
        perror("fgets input");
        fprintf(stderr, "%s: Error reading input\n", datetime_str ? datetime_str : "ERROR");
        free(datetime_str);
        client_worker_disconnect(worker);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }

    // Remove trailing newline character from fgets input if present
    size_t input_len = strlen(input_buffer);
    if (input_len > 0 && input_buffer[input_len - 1] == '\n') {
        input_buffer[input_len - 1] = '\0';
    }

    // 4. Create message_t to send
    message_to_send = message_create(MESSAGE_TYPE_NONE, strlen(input_buffer) + 1); // +1 for null terminator
    if (!message_to_send) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Failed to create message\n", datetime_str ? datetime_str : "ERROR");
        free(datetime_str);
        client_worker_disconnect(worker);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }
    strcpy((char *)message_to_send->data, input_buffer); // Copy input to message data
    message_to_send->header.length = strlen(input_buffer) + 1; // Include null terminator in length

    // 5. Send message to server
    client_status = client_worker_send(worker, message_to_send);
    if (client_status != CLIENT_WORKER_STATUS_SUCCESS) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Failed to send message to server: %d\n", datetime_str ? datetime_str : "ERROR", client_status);
        free(datetime_str);
        message_delete(message_to_send);
        client_worker_disconnect(worker);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }
    message_delete(message_to_send); // Message sent, free it

    // 6. Receive response from server
    received_message = client_worker_receive(worker); // Blocking receive
    if (!received_message) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Failed to receive message from server\n", datetime_str ? datetime_str : "ERROR");
        free(datetime_str);
        client_worker_disconnect(worker);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }

    // 7. Print server's response
    printf("Server response: %.*s\n", received_message->header.length, received_message->data);

    // 8. Clean up and disconnect
    message_delete(received_message); // Free received message
    client_status = client_worker_disconnect(worker);
    if (client_status != CLIENT_WORKER_STATUS_SUCCESS) {
        datetime_str = get_current_datetime_string();
        fprintf(stderr, "%s: Error during disconnection: %d\n", datetime_str ? datetime_str : "ERROR", client_status);
        free(datetime_str);
        client_worker_delete(worker);
        return EXIT_FAILURE;
    }
    client_worker_delete(worker);

    printf("Client finished successfully\n");
    return EXIT_SUCCESS;
}

// ********************************* //
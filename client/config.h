#ifndef CONFIG_H
#define CONFIG_H

// Server Configuration
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5000

// Client Configuration
#define CLIENT_PID_ARRAY {1000, 200, 300, 20}
#define CLIENT_NUM_PID 4

#define SERVER_NUM_PORT 5
#define SERVER_PORT_ARRAY {SERVER_PORT, SERVER_PORT + 1, SERVER_PORT + 2, SERVER_PORT + 3, SERVER_PORT + 4}

#define CLIENT_MODE CLIENT_MODE_NORMAL

#endif    // CONFIG_H
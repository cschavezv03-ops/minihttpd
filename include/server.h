#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>

#define BUFFER_SIZE 4096

// Differentiate server events from client events
typedef enum{
    LISTENER,
    CLIENT
}ConnectionType;

// Structure that represents a connection for managing partial sending
typedef struct{
    ConnectionType type;
    int fd;                             // Client socket file descriptor
    char output_buffer[BUFFER_SIZE];    // It keeps the pendant data to send
    size_t output_length;               // Total amount of prepared data
    size_t bytes_sent;                  // Total amount of successful data sent
    int read_closed;
}Connection;

int server(void);   //It is a function that starts the server and returns a socket descriptor if the server is running successfully, otherwise it returns -1. Whoever receives a valid descriptor is responsible for closing it when they are finished using it.

#endif // SERVER_H
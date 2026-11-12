#define _POSIX_C_SOURCE 200809L

#include <stdio.h>              // For printf, perror
#include <string.h>             // For memset
#include <sys/types.h>          // For socket, bind, listen, accept
#include <sys/socket.h>         // For socket, bind, listen, accept
#include <netdb.h>              // For getaddrinfo, freeaddrinfo
#include <arpa/inet.h>          // For htons
#include <unistd.h>             // For close, read, write, and sleep
#include <fcntl.h>              // For fcntl
#include "../include/server.h"  // Include the server header file for the server function declaration

#define IP_ADDRESS "localhost"  // Define the IP address for the server to bind to
#define PORT "8080"             // Define the port number for the server to listen on

//server() returns a file descriptor if the server is running, otherwise it returns -1
int server(void){

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(hints));   // Clear the hints structure
    hints.ai_family = AF_INET;          // Use IPv4
    hints.ai_socktype = SOCK_STREAM;    // Stream-oriented connection
    hints.ai_protocol = IPPROTO_TCP;    // Use TCP
    //hints.ai_flags = AI_PASSIVE;        // Server mode

    // Get address information for the server in a linked list of addrinfo structures (res)
    int status = getaddrinfo(IP_ADDRESS, PORT, &hints, &res);
    if(status != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status)); // Print error message if getaddrinfo fails
        return -1;
    }

    int nsd = -1; // Initialize the socket descriptor to -1
    // Iterate through the linked list of addrinfo structures to find a suitable address to bind to
    for(p = res; p != NULL; p = p->ai_next){
        // Create a socket using the address information in the current addrinfo structure
        int sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        
        if(sd < 0){
            perror("socket");  // Print error message if socket creation fails
            continue;          // Try the next addrinfo structure
        }

        // Bind the socket to the address in the current addrinfo structure
        if(bind(sd, p->ai_addr, p->ai_addrlen) < 0){
            perror("bind");     // Print error message if bind fails
            close(sd);          // Close the socket if bind fails
            continue;           // Try the next addrinfo structure
        } else {
            nsd = sd;   // Store the socket descriptor if bind is successful
            printf("Socket created and bound successfully. Socket descriptor: %d\n", nsd); // Print success message if socket creation and binding are successful
            break;      // Break the loop if socket creation and binding are successful
        }
    }

    freeaddrinfo(res); // Free the linked list of addrinfo structures

    if(nsd == -1){ // Check if no suitable address was found and the socket descriptor is still -1
        fprintf(stderr, "Failed to create socket\n"); // Print error message if no suitable address is found
        return -1;
    }

    // Non-blocking socket
    int flags = fcntl(nsd, F_GETFL);
    if (flags == -1){
        perror("F_GETFL");
        close(nsd);
        return -1;
    } else {
        int result = fcntl(nsd, F_SETFL, flags | O_NONBLOCK);
        if(result== -1){
            perror("F_SETFL");
            close(nsd);
            return -1;
        }
    }

    if(listen(nsd, 5) < 0){
        perror("listen");
        close(nsd); // Close the socket if listen fails
        return -1;
    } else {
        printf("Server is listening on %s:%s\n", IP_ADDRESS, PORT);
    }

    return nsd; // Return the socket descriptor if the server is running successfully
}
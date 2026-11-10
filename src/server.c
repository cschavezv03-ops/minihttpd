#define _POSIX_C_SOURCE 200809L

#include <stdio.h>          // For printf, perror
#include <string.h>         // For memset
#include <sys/types.h>      // For socket, bind, listen, accept
#include <sys/socket.h>     // For socket, bind, listen, accept
#include <netdb.h>          // For getaddrinfo, freeaddrinfo
#include <arpa/inet.h>      // For htons
#include <errno.h>          // For errno
#include <unistd.h>         // For close, read, write, and sleep

#define IP_ADDRESS "localhost" // Define the IP address for the server to bind to
#define PORT "8080"             // Define the port number for the server to listen on

//server() returns 1 if the server is running, otherwise it returns 0
int server(){

    // sockaddr_in struct is used to store the server's address information
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(hints));   // Clear the hints structure
    hints.ai_family = AF_INET;          // Use IPv4
    hints.ai_socktype = SOCK_STREAM;    // Stream-oriented connection
    hints.ai_protocol = IPPROTO_TCP;    // Use TCP
    hints.ai_flags = AI_PASSIVE;        // Server mode

    // Get address information for the server in a linked list of addrinfo structures (res)
    int status = getaddrinfo(IP_ADDRESS, PORT, &hints, &res);
    if(status != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status)); // Print error message if getaddrinfo fails
        return 0;
    }

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
            perror("bind"); // Print error message if bind fails
            continue;       // Try the next addrinfo structure
        }
        
        printf("Socket created and bound successfully\n"); // Print success message if socket creation and binding are successful
        freeaddrinfo(res); // Free the linked list of addrinfo structures
        break;
    }

    if(p == NULL){
        fprintf(stderr, "Failed to create socket\n"); // Print error message if no suitable address is found
        freeaddrinfo(res); // Free the linked list of addrinfo structures
        return 0;
    }

    return 1;
}
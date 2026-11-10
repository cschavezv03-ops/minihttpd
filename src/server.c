#define _POSIX_C_SOURCE 200809L

#include <stdio.h>          // For printf, perror
#include <string.h>         // For memset
#include <sys/types.h>      // For socket, bind, listen, accept
#include <sys/socket.h>     // For socket, bind, listen, accept
#include <netdb.h>          // For getaddrinfo, freeaddrinfo
#include <arpa/inet.h>      // For htons
#include <errno.h>          // For errno
#include <unistd.h>         // For close, read, write, and sleep

#define IP_ADDRESS "localhost"  // Define the IP address for the server to bind to
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

    int nsd = -1; // Initialize the socket descriptor to 0
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
            printf("Socket created and bound successfully\n"); // Print success message if socket creation and binding are successful
            break;      // Break the loop if socket creation and binding are successful
        }
    }

    freeaddrinfo(res); // Free the linked list of addrinfo structures

    if(p == NULL && nsd == -1){ // Check if no suitable address was found and the socket descriptor is still -1
        fprintf(stderr, "Failed to create socket\n"); // Print error message if no suitable address is found
        return 0;
    }

    if(listen(nsd, 5) < 0){
        close(nsd); // Close the socket if listen fails
        perror("listen");
        return 0;
    } else {
        printf("Server is listening on %s:%s\n", IP_ADDRESS, PORT);
    }

    // Iterative server loop to accept incoming connections and handle them one at a time
    // Demo server that echoes back received data to the client, it will be replaced with epoll in the future
    int accept_sd = -1; // Initialize the accept socket descriptor to -1
    while(1){
        accept_sd = accept(nsd, NULL, NULL); // Accept incoming connections (blocking call)
        if(accept_sd < 0){
            perror("accept");
            close(nsd);
            return 0;
        }
        printf("Accepted a connection\n");
        
        char buffer[1024];

        while(1){
            ssize_t bytes_received = recv(accept_sd, buffer, sizeof(buffer) - 1, 0);

            if(bytes_received < 0){
                perror("recv");
                break;
            }

            if(bytes_received == 0){
                printf("Client disconnected\n");
                break;
            }

            buffer[bytes_received] = '\0';
            printf("Received: %s\n", buffer);

            ssize_t bytes_sent = 0;

            while(bytes_sent < bytes_received){
                ssize_t n = send(accept_sd, buffer + bytes_sent, bytes_received - bytes_sent, MSG_NOSIGNAL);

                if(n <= 0){
                    perror("send");
                    break;
                }

                bytes_sent += n;
            }

            if(bytes_sent < bytes_received){
                break;
            }
        }

        close(accept_sd);
    }

    return 1;
}
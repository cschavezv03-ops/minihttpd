#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include "../include/server.h"

#define MAX_EVENTS 10

int event_loop(void){

    struct epoll_event registration = {0};
    struct epoll_event ready_events[MAX_EVENTS];
    struct epoll_event client_registration = {0};
    int client_fd;
    int server_fd;
    int epoll_fd;
    int n_events;
    int fd;
    int flags;
    int result;
    int socket_error;
    char buffer[1024];
    ssize_t bytes_received;
    socklen_t len;

    server_fd = server(); // Start the server and get the listening socket descriptor
    if (server_fd < 0) {
        fprintf(stderr, "Failed to start the server\n");
        return 1;
    }

    // Create an epoll instance for monitoring multiple file descriptors 
    epoll_fd = epoll_create1(EPOLL_CLOEXEC); // Create an epoll instance with the close-on-exec flag
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd); // Close the server socket if epoll creation fails
        return 1;
    }

    // Register the server socket with the epoll instance to monitor for incoming connections
    registration.events = EPOLLIN;     // Monitor for read events (incoming connections)
    registration.data.fd = server_fd;  // Store the server socket descriptor in the event data

    // Add the server socket to the epoll instance
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &registration) == 0){
        printf("Server socket registered with epoll successfully. Epoll file descriptor: %d\n", epoll_fd);
    }else{
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    while(1){ // Server functionality
        n_events = epoll_wait(epoll_fd, ready_events, MAX_EVENTS, -1);

        if(n_events == -1){
            if(errno == EINTR){
                continue;
            }
            perror("epoll_wait");
            close(server_fd);
            close(epoll_fd);
            return 1;
        }

        
        for (int i = 0; i < n_events; i++){ // Go over every event given by epoll_wait()
            fd = ready_events[i].data.fd;
            if(fd == server_fd){ // The listening socket is ready to accept at least one connection
                // Pending connections. Server event attendance (client acceptation)
                while(1){
                    client_fd = accept(server_fd, NULL, NULL);
                    if(client_fd >= 0){ // Client accepted
                        // Non blocking socket configuration
                        flags = fcntl(client_fd, F_GETFL);
                        if (flags == -1){
                            perror("F_GETFL");
                            close(client_fd);
                            continue;
                        } else {
                            result = fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                            if(result== -1){
                                perror("F_SETFL");
                                close(client_fd);
                                continue;
                            }
                        }
                        // Register client file descriptor socket into epoll
                        client_registration.events = EPOLLIN | EPOLLRDHUP;     // Monitor for read events (incoming connections)
                        client_registration.data.fd = client_fd;
                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_registration) == 0){
                            printf("Client socket registered with epoll successfully (fd=%d)\n", client_fd);
                        }else{
                            perror("epoll_ctl");
                            close(client_fd);
                            continue;
                        }
                    }else if(errno == EINTR){ // Call has been interrupted
                        printf("Call has been interrupted. Retrying acceptation...\n");
                        continue;
                    }else if (errno == EAGAIN || errno == EWOULDBLOCK) { // No pendant connections
                        printf("No pendant connections. Acceptance ends...\n");
                        break;
                    }else{
                        perror("client_fd");
                        break;
                    }
                }
            }else{
                // Client event. TCP operations attendance (sequential implementation)
                // Non blocking reception with EPOLLIN
                int close_client = 0;
                uint32_t events = ready_events[i].events;
                if(events & EPOLLIN){ // If the file descriptor includes EPOLLIN, try to receive bytes
                    for(;;){
                        bytes_received = recv(fd, buffer, sizeof(buffer), 0);
                        if(bytes_received > 0){
                            printf("Received %zd bytes\n", bytes_received);
                            printf("Data: %.*s\n", (int)bytes_received, buffer);
                            continue;
                        }
                        
                        if (bytes_received == 0){
                            printf("Client stopped sending\n");
                            close_client = 1;
                            break;
                        }
                        
                        if(errno == EINTR){
                            printf("Reception interrupted\n");
                            continue;
                        }

                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            printf("No more available data\n");
                            break;
                        }

                        perror("recv");
                        close_client = 1;
                        break;
                    }
                }

                if(events & EPOLLERR){ //Handle connection failure
                    //Check error with getsockopt()
                    socket_error = 0;
                    len = sizeof(socket_error);

                    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &len) == -1){
                        perror("getsockopt");
                    }else{
                        strerror(socket_error);
                        fprintf(stderr, "Socket error: %d\n", socket_error);
                    }
                    
                }

                if(events & EPOLLHUP){ //Don't eliminate data or pendant responses (in HTTP)
                    printf("Connection hangup: fd=%d\n", fd);
                    close_client = 1;
                }

                if(events & EPOLLRDHUP){ //Allow to finish a pendant response before the closing (in HTTP)
                    printf("Client finished sending: fd=%d\n", fd);
                    close_client = 1;
                }

                if (close_client == 1) {
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                        perror("epoll_ctl DEL");
                    }

                    if (close(fd) == -1) {
                        perror("close");
                    }
                }
            }
        }
    }

    // Free up resources
    close(server_fd);
    close(epoll_fd);

    return 0;
}

int main(void) {
    // Start the event loop to handle incoming connections and events
    return event_loop();
}
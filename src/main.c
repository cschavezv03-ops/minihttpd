#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include "../include/server.h"

#define MAX_EVENTS 10

int event_loop(void){

    struct epoll_event registration = {0};
    struct epoll_event ready_events[MAX_EVENTS];
    struct epoll_event client_registration = {0};

    int server_fd = server(); // Start the server and get the listening socket descriptor
    if (server_fd < 0) {
        fprintf(stderr, "Failed to start the server\n");
        return 1;
    }

    // Create an epoll instance for monitoring multiple file descriptors 
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC); // Create an epoll instance with the close-on-exec flag
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

    while(1){
        int n_events = epoll_wait(epoll_fd, ready_events, MAX_EVENTS, -1);

        if(n_events == -1){
            if(errno == EINTR){
                continue;
            }
            perror("epoll_wait");
            close(server_fd);
            close(epoll_fd);
            return 1;
        }

        for (int i = 0; i < n_events; i++){
            int fd = ready_events[i].data.fd;
            if(fd == server_fd){ // The listening socket is ready to accept at least one connection
                // Pending connections. Server event attendance (client acceptation)
                while(1){
                    int client_fd = accept(server_fd, NULL, NULL);
                    if(client_fd >= 0){ // Client accepted
                        // Non blocking socket configuration
                        int flags = fcntl(client_fd, F_GETFL);
                        if (flags == -1){
                            perror("F_GETFL");
                            close(client_fd);
                            break;
                        } else {
                            int result = fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                            if(result== -1){
                                perror("F_SETFL");
                                close(client_fd);
                                break;
                            }
                        }
                        // Register client file descriptor socket into epoll
                        client_registration.events = EPOLLIN;     // Monitor for read events (incoming connections)
                        client_registration.data.fd = client_fd;
                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_registration) == 0){
                            printf("Client socket registered with epoll successfully (fd=%d)\n", client_fd);
                        }else{
                            perror("epoll_ctl");
                            close(client_fd);
                            break;
                        }
                    }else if(errno == EINTR){ // Call has been interrupted
                        continue;
                    }else if (errno == EAGAIN || errno == EWOULDBLOCK) { // No pendant connections
                        break;
                    }else{
                        perror("client_fd");
                        break;
                    }
                }
            }else{
                // Client event. TCP operations attendance
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
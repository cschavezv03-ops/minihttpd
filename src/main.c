#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include "../include/server.h"

#define MAX_EVENTS 10

int event_loop(void){

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
    struct epoll_event registration = {0};
    registration.events = EPOLLIN;     // Monitor for read events (incoming connections)
    registration.data.fd = server_fd;  // Store the server socket descriptor in the event data

    struct epoll_event ready_events[MAX_EVENTS];

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
            if(fd == server_fd){
                // Pending connections. Server event attendance
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
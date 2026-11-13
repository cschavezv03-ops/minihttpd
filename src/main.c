#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../include/server.h"

#define MAX_EVENTS 10

int event_loop(void){

    //EPOLL
    struct epoll_event registration;
    memset(&registration, 0, sizeof(registration));
    
    struct epoll_event ready_events[MAX_EVENTS];
    memset(ready_events, 0, sizeof(ready_events));

    struct epoll_event client_registration;
    memset(&client_registration, 0, sizeof(client_registration));

    struct epoll_event send_to;
    memset(&send_to, 0, sizeof(send_to));

    //CONNECTION
    Connection listener;
    memset(&listener, 0, sizeof(listener));

    //VARIABLES
    int client_fd;
    int server_fd;
    int epoll_fd;
    int n_events;
    int flags;
    int result;
    int fd;
    int socket_error;
    int pending_output;
    ssize_t bytes_received;
    ssize_t n_sent_bytes;
    socklen_t len;

    server_fd = server(); // Start the server and get the listening socket descriptor
    if (server_fd < 0) {
        fprintf(stderr, "Failed to start the server\n");
        return 1;
    }

    listener.type = LISTENER;
    listener.fd = server_fd;

    // Create an epoll instance for monitoring multiple file descriptors 
    epoll_fd = epoll_create1(EPOLL_CLOEXEC); // Create an epoll instance with the close-on-exec flag
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd); // Close the server socket if epoll creation fails
        return 1;
    }

    // Register the server socket with the epoll instance to monitor for incoming connections
    registration.events = EPOLLIN;     // Monitor for read events (incoming connections)
    registration.data.ptr = &listener;  // Store the server socket descriptor in the event data

    // Add the server socket to the epoll instance
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &registration) == 0){
        printf("Server socket registered with epoll successfully. Epoll file descriptor: %d\n\n", epoll_fd);
    }else{
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    while(1){ // Server functionality (loop)
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
            Connection *current_connection = (Connection *)ready_events[i].data.ptr;
            if(current_connection->type == LISTENER){ // The listening socket is ready to accept at least one connection
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
                        // Allocate dynamic memory for the client and initialize the structure to 0
                        Connection *client_connection = (Connection *)calloc(1, sizeof(Connection));
                        if(client_connection == NULL){
                            perror("calloc");
                            close(client_fd);
                            continue;
                        }
                        //Configure client fields
                        client_connection->type = CLIENT;
                        client_connection->fd = client_fd;
                        // Register client file descriptor socket into epoll
                        client_registration.events = EPOLLIN | EPOLLRDHUP;     // Monitor for read events (incoming connections)
                        client_registration.data.ptr = client_connection;
                        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_registration) == 0){
                            printf("\nClient socket registered with epoll successfully (fd=%d)\n", client_fd);
                            printf("Event: type=%d, fd=%d\n", client_connection->type, client_connection->fd);
                        }else{
                            perror("epoll_ctl");
                            close(client_fd);
                            free(client_connection);
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
            }else if(current_connection->type == CLIENT){
                // Client event. TCP operations attendance (sequential implementation)
                // Non blocking reception with EPOLLIN
                int close_client = 0;
                fd = current_connection->fd;
                uint32_t events = ready_events[i].events;
                if(events & EPOLLIN){ // If the file descriptor includes EPOLLIN, try to receive bytes
                    while(1){
                        
                        // Add received bytes to the buffer of the current_connection structure
                        bytes_received = recv(fd, current_connection->output_buffer, BUFFER_SIZE, 0);
                        if(bytes_received > 0){
                            // Receive and prepare bytes for sending
                            current_connection->output_length = (size_t)bytes_received;
                            current_connection->bytes_sent = 0;
                            send_to.events = EPOLLOUT | EPOLLRDHUP;
                            send_to.data.ptr = current_connection;
                            if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &send_to) == -1){
                                perror("epoll_ctl MOD");
                                close_client = 1;
                                break;
                            }
                            printf("\nReceived %zd bytes from fd=%d\n", bytes_received, fd);
                            printf("Data: %.*s\n", (int)bytes_received, current_connection->output_buffer);
                            break;
                        }
                        
                        if (bytes_received == 0){
                            printf("\nClient fd=%d stopped sending\n", fd);
                            current_connection->read_closed = 1;
                            break;
                        }
                        
                        if(errno == EINTR){
                            printf("Reception from fd=%d interrupted\n", fd);
                            continue;
                        }

                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            printf("No more available data from fd=%d\n", fd);
                            break;
                        }

                        perror("recv");
                        close_client = 1;
                        break;
                    }
                }

                if(events & EPOLLOUT){
                    // Send data to destination
                    if(current_connection->bytes_sent < current_connection->output_length){
                        n_sent_bytes = send(fd,
                            current_connection->output_buffer + current_connection->bytes_sent,
                            current_connection->output_length - current_connection->bytes_sent,
                            MSG_NOSIGNAL);
                        if(n_sent_bytes > 0){
                            // Some bytes were sent successfully
                            current_connection->bytes_sent += (size_t)n_sent_bytes;
                        }else if(n_sent_bytes == 0){
                            // No progress and there are some pendant bytes to send
                            fprintf(stderr, "send: No progress on fd=%d\n", fd);
                            close_client = 1;
                        }else if(n_sent_bytes == -1){
                            if(errno == EINTR){
                                printf("Sending to fd=%d interrupted. Retrying...\n", fd);
                            }else if(errno == EAGAIN || errno == EWOULDBLOCK){
                                printf("Potential pending data. Retrying...\n");
                            }
                        }else{
                            // Unrepairable error
                            perror("send");
                            close_client = 1;
                        }
                    }

                    if(!close_client && current_connection->output_length > 0 && current_connection->bytes_sent == current_connection->output_length){
                        // No pendant output to send
                        // Sent terminated but client still connected
                        if(current_connection->read_closed == 0){
                            printf("All bytes (%d) were sent\n", (int)current_connection->output_length);
                            printf("Sent data to fd=%d: %.*s\n", fd, (int)n_sent_bytes, current_connection->output_buffer);
                            current_connection->output_length = 0;
                            current_connection->bytes_sent = 0;
                            // Reset to EPOLLIN to wait for data again
                            send_to.events = EPOLLIN | EPOLLRDHUP;
                            send_to.data.ptr = current_connection;
                            // Restore EPOLLIN flag into the file descriptor
                            if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &send_to) == -1){
                                perror("epoll_ctl MOD");
                                close_client = 1;
                            }
                        }
                    }
                }

                if(events & EPOLLERR){ //Handle connection failure
                    //Check error with getsockopt()
                    socket_error = 0;
                    len = sizeof(socket_error);

                    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &len) == -1){
                        perror("getsockopt");
                    }else{
                        fprintf(stderr, "Socket error: %s\n", strerror(socket_error));
                    }
                    close_client = 1;
                }

                if(events & EPOLLHUP){ //Don't eliminate data or pendant responses (in HTTP)
                    printf("Connection hangup: fd=%d\n", fd);
                    current_connection->read_closed = 1;
                }

                if(events & EPOLLRDHUP){ //Allow to finish a pendant response before the closing (in HTTP)
                    printf("Client finished connection: fd=%d\n", fd);
                    current_connection->read_closed = 1;
                }

                pending_output = current_connection->bytes_sent < current_connection->output_length;
                if(!pending_output && current_connection->read_closed){
                    close_client = 1;
                }

                if (close_client == 1) {
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                        perror("epoll_ctl DEL");
                    }

                    if (close(fd) == -1) {
                        perror("close");
                    }

                    free(current_connection);
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
#include <stdio.h>
#include "../include/server.h"

int main(void) {
    int server_fd = server();
    if (server_fd < 0) {
        fprintf(stderr, "Failed to start the server\n");
        return 1;
    }
    return 0;
}
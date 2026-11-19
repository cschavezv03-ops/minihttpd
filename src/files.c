#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#include "../include/files.h"

int open_static_file(const char *uri, int *file_fd, size_t *file_size) {
    if(!uri || !file_fd || !file_size){
        return 500;
    }
    *file_fd = -1;
    *file_size = 0;
    if(uri[0] != '/'){
        return 400;
    }
    char root[PATH_MAX], candidate[PATH_MAX], resolved[PATH_MAX];
    // Find the absolute path of www/
    if(!realpath("www", root)){
        return 500;
    }
    // Build the requested file path
    int n;
    if(strcmp(uri, "/") == 0){
        n = snprintf(candidate, sizeof(candidate),
                    "%s/index.html", root);
    }else{
        n = snprintf(candidate, sizeof(candidate),
                    "%s%s", root, uri);
    }

    if(n < 0 || (size_t)n >= sizeof(candidate)){
        return 400;
    }
    // Resolve the actual path
    if(!realpath(candidate, resolved)){
        if (errno == ENOENT || errno == ENOTDIR){
            return 404;
        }
        if (errno == EACCES || errno == ELOOP){
            return 403;
        }
        return 500;
    }
    // Ensure the file is inside www/
    size_t root_len = strlen(root);
    size_t resolved_len = strlen(resolved);
    if (resolved_len <= root_len || strncmp(root, resolved, root_len) != 0 || resolved[root_len] != '/'){
        return 403;
    }

    // Open the file
    int fd = open(resolved, O_RDONLY | O_CLOEXEC);
    if (fd < 0){
        if (errno == ENOENT) return 404;
        if (errno == EACCES) return 403;
        return 500;
    }
    // Check the file and obtain its size
    struct stat info;

    if(fstat(fd, &info) < 0){
        close(fd);
        return 500;
    }

    if (!S_ISREG(info.st_mode)) {
        close(fd);
        return 403;
    }

    if (info.st_size < 0 ||
        (uintmax_t)info.st_size > SIZE_MAX) {
        close(fd);
        return 500;
    }

    *file_fd = fd;
    *file_size = (size_t)info.st_size;

    return 200;
}
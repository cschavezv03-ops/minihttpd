#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int open_static_file(const char *uri, int *file_fd, size_t *file_size);

#endif
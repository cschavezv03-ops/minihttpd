#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_METHOD_SIZE 8
#define HTTP_URI_SIZE 2048
#define HTTP_VERSION_SIZE 16

//Divide the first HTTP Request Line
typedef struct {
    char method[HTTP_METHOD_SIZE];
    char uri[HTTP_URI_SIZE];
    char version[HTTP_VERSION_SIZE];
} HttpRequest;

int parse_request_line(
    const char *line,
    size_t line_length,
    HttpRequest *request
);

#endif // HTTP_H
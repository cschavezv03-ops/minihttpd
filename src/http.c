#include <stdio.h>
#include <string.h>
#include "../include/http.h"

int parse_request_line(const char *line, size_t line_length, HttpRequest *request){
    // GET /index.html HTTP/1.1 parsing
    if(line_length == 0){
        fprintf(stderr, "Empty request line\n");
        return -1;
    }

    if(line == NULL || request == NULL){
        fprintf(stderr, "Invalid null pointer\n");
        return -1;
    }

    // Method extraction
    // Locate first space (char = ' ') in line
    const char *first_space = (const char *)memchr(line, ' ', line_length);

    if(first_space == NULL){
        puts("Missing Method separator");
        return -1;
    }

    size_t method_length = (size_t)(first_space - line);
    
    // Method length validation
    if(method_length == 0 || method_length >= HTTP_METHOD_SIZE){
        fprintf(stderr, "Invalid method length\n");
        return -1;
    }

    memcpy(request->method, line, method_length);
    request->method[method_length] = '\0';
    printf("Method: %s\n", request->method);

    // URI extraction
    const char *uri_start = first_space + 1;
    // Calculate remaining bytes from URI start
    size_t consumed = (size_t)(uri_start - line);
    size_t remaining = line_length - consumed;
    const char *second_space = (const char *)memchr(uri_start, ' ', remaining); // Locate second space (char = ' ') in line

    if(second_space == NULL){
        fprintf(stderr, "Missing URI separator\n");
        return -1;
    }

    size_t uri_length = (size_t)(second_space - uri_start);

    if (uri_length == 0 || uri_length >= HTTP_URI_SIZE) {
        fprintf(stderr, "Invalid URI length\n");
        return -1;
    }

    // If something like this comes: GET /index.html\0hidden HTTP/1.1
    if (memchr(uri_start, '\0', uri_length) != NULL) {
        fprintf(stderr, "Invalid NUL byte in URI\n");
        return -1;
    }

    memcpy(request->uri, uri_start, uri_length);
    request->uri[uri_length] = '\0';

    // Demand "/" at the begining of the URI
    if (request->uri[0] != '/') {
        fprintf(stderr, "Invalid URI format\n");
        return -1;
    }

    printf("URI: %s\n", request->uri);

    // HTTP version extraction
    const char *version_start = second_space + 1;
    size_t version_length = line_length - (size_t)(version_start - line);

    if (version_length == 0 || version_length >= HTTP_VERSION_SIZE) {
        fprintf(stderr, "Invalid HTTP version length\n");
        return -1;
    }

    // No additional element after HTTP/1.1
    if (memchr(version_start, ' ', version_length) != NULL) {
        fprintf(stderr, "Unexpected extra field\n");
        return -1;
    }

    memcpy(request->version, version_start, version_length);
    request->version[version_length] = '\0';

    // HTTP/1.1 version validation
    if(strcmp(request->version, "HTTP/1.1") != 0){
        fprintf(stderr, "Unsupported HTTP version\n");
        return -1;
    }

    printf("HTTP version: %s\n", request->version);

    return 0;
}



/*int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s \"METHOD /URI HTTP/1.1\"\n", argv[0]);
        return 1;
    }

    HttpRequest request = {0};

    int result = parse_request_line(
        argv[1],
        strlen(argv[1]),
        &request
    );

    if (result != 0) {
        fprintf(stderr, "\nInvalid HTTP request line\n");
        return 1;
    }

    printf("\nHTTP request parsed successfully!\n");

    return 0;
}*/
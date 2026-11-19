#include <stdio.h>
#include <string.h>             // memchr, memcpy, etc...
#include <strings.h>            // strncasecmp()
#include "../include/http.h"

// It returns 1 if the character is permited, instead of that, returns 0
static int is_http_token_char(unsigned char c){
    // A-Za-z0-9
    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')){
        return 1;
    }
    // !#$%&'*+-.^_`|~ without accepting \0
    switch (c) {
        case '!': case '#': case '$': case '%':
        case '&': case '\'': case '*': case '+':
        case '-': case '.': case '^': case '_':
        case '`': case '|': case '~':
            return 1;
        default:
            return 0;
    }
}

// It returns the corresponding status code
static const char *http_status_code(int status) {
    if(status != 200 && status != 400 && status != 403 && status != 404 && status != 405 && status != 500){
        return "Not admitted code";
    }
    
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default:  return "Internal Server Error";
    }
}

int parse_request_line(const char *line, size_t line_length, HttpRequest *request){
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

    // Validate method valid characters
    for (size_t i = 0; i < method_length; i++) {
        if (!is_http_token_char((unsigned char)line[i])) {
            fprintf(stderr, "Invalid HTTP method\n");
            return -1;
        }
    }
    
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

    // Validate URI valid characters
    for (size_t i = 0; i < uri_length; i++) {
        unsigned char c = (unsigned char)uri_start[i];

        if (c <= 32 || c == 127) {
            fprintf(stderr, "Invalid URI character\n");
            return -1;
        }
    }

    if (uri_length == 0 || uri_length >= HTTP_URI_SIZE) {
        fprintf(stderr, "Invalid URI length\n");
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

// HTTP header parsing
int parse_header(const char *line, size_t line_length, HttpRequest *request){
    if(line_length == 0){
        fprintf(stderr, "Empty request line\n");
        return -1;
    }

    if(line == NULL || request == NULL){
        fprintf(stderr, "Invalid null pointer\n");
        return -1;
    }
    
    // Separator ':' localization
    const char *colon = (const char *)memchr(line, ':', line_length);
    if(colon == NULL){
        printf("Missing colon\n");
        return -1;
    }

    size_t name_length = (size_t)(colon - line);

    if(name_length == 0){
        fprintf(stderr,"Missing header name\n");
        return -1;
    }

    // Header name verification with valid characters
    for (size_t i = 0; i < name_length; i++) {
        if (!is_http_token_char((unsigned char)line[i])) {
            fprintf(stderr, "Invalid HTTP header name\n");
            return -1;
        }
    }   

    // Remove spaces from the value
    const char *value_start = colon + 1;
    const char *value_end = line + line_length;

    while (value_start < value_end && (*value_start == ' ' || *value_start == '\t')) {
        // Skip initial spaces
        value_start++;
    }

    while (value_end > value_start && (value_end[-1] == ' ' || value_end[-1] == '\t')) {
        // Skip final spaces
        value_end--;
    }

    size_t value_length = (size_t)(value_end - value_start);

    // Headers value verification with valid characters
    for (size_t i = 0; i < value_length; i++) {
        unsigned char c = (unsigned char)value_start[i];

        if ((c < 32 && c != '\t') || c == 127) {
            fprintf(stderr, "Invalid HTTP header value\n");
            return -1;
        }
    }

    // Compare without distinguish upper or lower case
    if(name_length == 4 && strncasecmp(line, "Host", 4) == 0){
        // Host header
        if(value_length == 0 || value_length >= sizeof(request->host)){
            fprintf(stderr, "Invalid Host length\n");
            return -1;
        }
        if(request->host_seen == 1){
            fprintf(stderr, "No more than one host allowed\n");
            return -1;
        }
        memcpy(request->host, value_start, value_length);
        request->host[value_length] = '\0';
        request->host_seen = 1;
    }else if(name_length == 10 && strncasecmp(line, "Connection", 10) == 0){
        // Connection header
        if(value_length >= sizeof(request->connection)){
            return -1;
        }
        memcpy(request->connection, value_start, value_length);
        request->connection[value_length] = '\0';
    }else if(name_length == 10 && strncasecmp(line, "User-agent", 10) == 0){
        // User-agent header
        if(value_length >= sizeof(request->user_agent)){
            return -1;
        }
        memcpy(request->user_agent, value_start, value_length);
        request->user_agent[value_length] = '\0';
    }else{
        // Ignore valid headers but with valid sintaxis
    }

    return 0;
}

int parse_http_request(const char *data, size_t data_length, size_t *bytes_consumed, HttpRequest *request){
    // Recognize that \r\n separates lines and that \r\n\r\n terminates headings
    if(data == NULL || request == NULL || bytes_consumed == NULL){
        fprintf(stderr, "Invalid null pointer\n");
        return HTTP_PARSE_ERROR;
    }

    *bytes_consumed = 0;
    
    if(data_length == 0){
        return HTTP_PARSE_INCOMPLETE;
    }

    memset(request, 0, sizeof(*request));

    const char *newline = (const char *)memchr(data, '\n', data_length);

    if(newline == NULL){
        return HTTP_PARSE_INCOMPLETE;
    }

    // Find the first \r\n (Request line)
    if (newline == data || newline[-1] != '\r') {
        fprintf(stderr, "Invalid HTTP line ending\n");
        return HTTP_PARSE_ERROR;
    }

    size_t content_length = (size_t)(newline - data - 1);

    // Process the first line
    if (parse_request_line(data, content_length, request) != 0) {
        return HTTP_PARSE_ERROR;
    }

    // Go to the headers
    const char *cursor = newline + 1;
    const char *next_line;

    // Find \r\n into header lines and parse
    for(int i = 0; i <= HTTP_NUM_HEADERS; i++){
        size_t remaining = data_length - (size_t)(cursor - data);

        next_line = (const char *)memchr(cursor, '\n', remaining);

        // If there's no more data, request is incomplete
        if(next_line == NULL){
            return HTTP_PARSE_INCOMPLETE;
        }

        // Find the CRLF delimiter
        if (next_line == cursor || next_line[-1] != '\r') {
            fprintf(stderr, "Invalid HTTP line ending\n");
            return HTTP_PARSE_ERROR;
        }

        size_t header_length = (size_t)(next_line - cursor - 1);
        if (header_length == 0) {
            if (request->host_seen != 1) {
                fprintf(stderr, "Missing Host header\n");

                return HTTP_PARSE_ERROR;
            }
            *bytes_consumed = (size_t)((next_line + 1) - data);
            printf("Bytes consumed: %zu\n", *bytes_consumed);
            return HTTP_PARSE_OK;
        }

        if (i == HTTP_NUM_HEADERS) {
            fprintf(stderr, "Too many HTTP headers\n");
            return HTTP_PARSE_ERROR;
        }

        if(parse_header(cursor, header_length, request) != 0){
            return HTTP_PARSE_ERROR;
        }
        cursor = next_line + 1;
    }
    return HTTP_PARSE_ERROR;
}

int build_http_header(char *buffer, size_t capacity, int status, const char *content_type, size_t content_length, int keep_alive, size_t *header_length) {
    if (buffer == NULL || content_type == NULL ||
        header_length == NULL || capacity == 0) {
        return -1;
    }

    *header_length = 0;

    const char *reason = http_status_code(status);

    int n = snprintf(buffer,capacity,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n",
        status,
        reason,
        content_type,
        content_length,
        keep_alive ? "keep-alive" : "close"
    );

    if (n < 0 || (size_t)n >= capacity) {
        return -1;
    }

    *header_length = (size_t)n;

    return 0;
}

/*int main(void) {
    char buffer[512];
    size_t header_length = 0;

    int result = build_http_header(
        buffer,
        sizeof(buffer),
        200,
        "text/html",
        13,
        1,
        &header_length
    );

    if (result != 0) {
        fprintf(stderr, "Failed to build HTTP header\n");
        return 1;
    }

    printf("%s", buffer);
    printf("Header length: %zu\n", header_length);

    return 0;
}*/
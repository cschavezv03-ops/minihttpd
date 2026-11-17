#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_METHOD_SIZE 8          // GET  
#define HTTP_URI_SIZE 2048          // /index.html
#define HTTP_VERSION_SIZE 16        // HTTP/1.1
#define HTTP_HOST_SIZE 256          // Host: localhost:8080   
#define HTTP_CONNECTION_SIZE 32     // Connection: keep-alive
#define HTTP_USER_AGENT_SIZE 256    // User-Agent: curl/8.0
#define HTTP_NUM_HEADERS 3         // Number of HTTP/1.1 headers

//Divide the first HTTP Request Line
typedef struct {
    char method[HTTP_METHOD_SIZE];
    char uri[HTTP_URI_SIZE];
    char version[HTTP_VERSION_SIZE];
    char host[HTTP_HOST_SIZE];
    int host_seen;
    char connection[HTTP_CONNECTION_SIZE];
    char user_agent[HTTP_USER_AGENT_SIZE];
} HttpRequest;

typedef enum {
    HTTP_PARSE_ERROR = -1,
    HTTP_PARSE_INCOMPLETE = 0,
    HTTP_PARSE_OK = 1
} HttpParseResult;

// GET /index.html HTTP/1.1 parsing
int parse_request_line(
    const char *line,
    size_t line_length,
    HttpRequest *request
);

// Headers parsing (Host, Connection, User-agent)
int parse_header(
    const char *line,
    size_t line_length,
    HttpRequest *request
);

// It receives a complete request, identify its lines, and call the existing analyzers.
int parse_http_request(
    const char *data,
    size_t data_length,
    HttpRequest *request
);
#endif // HTTP_H
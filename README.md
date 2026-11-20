# MiniHTTPd

A basic HTTP/1.1 server written in C for Linux, without external HTTP libraries. This project explores TCP communication, HTTP request processing, and concurrent client handling using `epoll` in a single thread.

## Features

- Handles `GET` requests and serves static files from `www/`.
- Supports HTML, CSS, JavaScript, PNG, and JPG/JPEG files with their corresponding MIME types.
- Parses the HTTP/1.1 request line and the `Host`, `Connection`, and `User-Agent` headers.
- Uses event-driven concurrency (`epoll`) and non-blocking sockets.
- Supports persistent connections (*keep-alive*) to reuse the same TCP connection.
- Returns HTTP status codes `200`, `400`, `403`, `404`, `405`, and `500`.
- Validates paths with `realpath()` to restrict access to `www/`, checks buffer limits, and uses `snprintf()` for bounded header construction.

## Requirements

- Linux (developed and tested on Fedora).
- GCC and Make.

## Build and Run

From the repository root:

```bash
make clean && make
./minihttpd
```

The server listens on `localhost:8080`. You can also start it with `make run` after building it.

To remove the compiled executable:

```bash
make clean
```

## Testing

With the server running, open the following address in your browser:

<http://127.0.0.1:8080/>

Alternatively, request resources from another terminal:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/style.css
curl -i http://127.0.0.1:8080/script.js
curl -o /tmp/image.png http://127.0.0.1:8080/image.png
```

Test error responses:

```bash
curl -i http://127.0.0.1:8080/nonexistent.html
curl -i -X POST http://127.0.0.1:8080/
```

The expected responses are `404 Not Found` and `405 Method Not Allowed`, respectively. The `/` path maps to `www/index.html`. Requested resources must exist in the `www/` directory.

## Project Structure

```text
minihttpd/
├── Makefile
├── README.md
├── include/
│   ├── server.h
│   ├── http.h
│   ├── files.h
│   └── mime.h
├── src/
│   ├── main.c
│   ├── server.c
│   ├── http.c
│   ├── files.c
│   └── mime.c
└── www/
    ├── index.html
    ├── style.css
    ├── script.js
    └── image.png
```

- `server.c`: creates and configures the TCP listening socket.
- `main.c`: coordinates `epoll`, request reception, and response and file transmission.
- `http.c`: parses requests and builds HTTP headers.
- `files.c`: resolves and validates paths, opens files, and retrieves their sizes.
- `mime.c`: determines the content type based on the file extension.
- `www/`: contains the static resources served by MiniHTTPd.

## How It Works

The server registers its listening socket and client sockets with `epoll`. `EPOLLIN` indicates when connections can be accepted or requests can be received, while `EPOLLOUT` allows the server to continue sending responses without blocking the thread. For a valid `GET` request, the server locates the file within `www/`, determines its MIME type and size, sends the HTTP headers, and transfers the content using `sendfile()`. With *keep-alive*, another request can be handled over the same TCP connection.

## Scope

MiniHTTPd is an academic implementation intended to demonstrate HTTP/1.1 and event-driven concurrency. It only supports `GET` and static files; it does not include TLS, dynamic applications, or the security and performance features of a production web server.

## References

- [RFC 9110 - HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110)
- [RFC 9112 - HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112)

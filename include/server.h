#ifndef SERVER_H
#define SERVER_H

int server(void);   //It is a function that starts the server and returns a socket descriptor if the server is running successfully,
                //otherwise it returns -1. Whoever receives a valid descriptor is responsible for closing it when they are finished using it.

#endif // SERVER_H
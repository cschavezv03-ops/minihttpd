# gcc compiler
CC = gcc

# executable name

TARGET = minihttpd

# compiler flags

CFLAGS = -Wall -Wextra -Werror -Iinclude

# source files

SRC = src/main.c
SRC += src/server.c

# executable command

.PHONY: all run clean

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
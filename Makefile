# gcc compiler
CC = gcc

# executable name

TARGET = minihttpd

# compiler flags

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

# source files

SRC = src/main.c

# executable command

.PHONY: all run clean

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
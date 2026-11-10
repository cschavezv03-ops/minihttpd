# gcc compiler
CC = gcc

# executable name

TARGET = main

# compiler flags

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

# source files

SRC = src/main.c

# executable command

all:
	$(CC) $(CFLAGS) $(SRC) -o src/$(TARGET) && ./src/$(TARGET)
clean:
	rm -f src/$(TARGET)
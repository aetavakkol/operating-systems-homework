# Compiler to use
CC = gcc

# Options
CFLAGS = -Wall -Werror -g -ansi

# Source files
SOURCES = process.c

# Executable file
OUTPUT = process

.PHONY:		clean

all:		clean compile

compile:
			$(CC) $(SOURCES) $(CFLAGS) -o $(OUTPUT)

clean:
			$(RM) -r $(OUTPUT)

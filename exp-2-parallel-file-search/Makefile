# Compiler to use
CC = gcc

# Options
CFLAGS = -Wall -ansi -Werror -g

# Libraries
LIBS = -lpthread

# Source files
SOURCES = main.c minion.c

# Executable files
EXECUTABLES = $(patsubst %.c,%,$(SOURCES))

# Build directory
BUILD_DIR = build

all:		dir $(EXECUTABLES)

%: %.c
			$(CC) $< -o $(BUILD_DIR)/$(notdir $@) $(CFLAGS) $(LIBS)

dir:
			mkdir -p $(BUILD_DIR)

clean:
			$(RM) -r $(BUILD_DIR)
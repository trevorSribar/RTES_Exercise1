# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11

# Target executable
TARGET = loadGenSimFib

# Source files
SRCS = loadGenSimFib.c

# Default target
all: $(TARGET)

# Build rule
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean up build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all clean

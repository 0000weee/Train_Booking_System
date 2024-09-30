# Define the compiler and flags
CC = gcc
CFLAGS = -Wall

# Define the source file
SRC = server.c

# Targets
READ_TARGET = read_server
WRITE_TARGET = write_server

# Default target to build both
all: $(READ_TARGET) $(WRITE_TARGET)

# Compile read_server
$(READ_TARGET): $(SRC)
	$(CC) $(CFLAGS) -D READ_SERVER -o $(READ_TARGET) $(SRC)

# Compile write_server
$(WRITE_TARGET): $(SRC)
	$(CC) $(CFLAGS) -D WRITE_SERVER -o $(WRITE_TARGET) $(SRC)

# Clean up the executables
clean:
	rm -f $(READ_TARGET) $(WRITE_TARGET)

.PHONY: all clean
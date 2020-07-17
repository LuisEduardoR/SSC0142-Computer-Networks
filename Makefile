# C compiler to be used.
CC = gcc

# General flags for the compliler.
FLAGS = -Wall -O

# Linker related flags for the compiler.
LINKER_FLAGS = -lstdc++ -lpthread -lrt

# Directories with the source files.
MAIN_SRC_DIR = ./src
CLT_SRC_DIR = ./src/client
SRV_SRC_DIR = ./src/server

# Final compiled executable name.
OUTPUT = trabalho-redes

# Compiles all object files and links them to make the final executable.
all:
	$(CC) $(MAIN_SRC_DIR)/*.cpp $(CLT_SRC_DIR)/*.cpp $(SRV_SRC_DIR)/*.cpp $(FLAGS) $(LINKER_FLAGS) -o $(OUTPUT)

# Tries running the compiled executable if it's name wasn't changed
run:
	./$(OUTPUT)



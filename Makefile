# C compiler to be used.
CC = gcc
# General flags for the compliler.
FLAGS = -Wall -O
# Linker related flags for the compiler.
LINKER_FLAGS = -lstdc++ -lpthread -lrt
# Directory with the source files.
SRC_DIR = ./src
# Directory with the object files.
OBJ_DIR = ./obj
# Final compiled executable name.
OUTPUT = trabalho-redes

# Creates an object from the main program funcion.
main:
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(SRC_DIR)/main.cpp $(FLAGS) $(LINKER_FLAGS) -o $(OBJ_DIR)/main.o
# Creates the object with with the code handle a client instance.
client:
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(SRC_DIR)/client/*.cpp $(FLAGS) $(LINKER_FLAGS) -o $(OBJ_DIR)/client.o
# Creates the object with the code to handle a server instance.
server:
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(SRC_DIR)/server/*.cpp $(FLAGS) $(LINKER_FLAGS) -o $(OBJ_DIR)/server.o
# Creates an object with the code to handle the messaging between server and clients.
messaging:
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(SRC_DIR)/messaging/*.cpp $(FLAGS) $(LINKER_FLAGS) -o $(OBJ_DIR)/messaging.o
# Links all objects on the $(OBJ_DIR) to make the final executable.
link:
	$(CC) $(OBJ_DIR)/*.o $(FLAGS) $(LINKER_FLAGS) -o $(OUTPUT)
# Compiles all object files and links them to make the final executable.
all: main client server messaging link
# Deletes all object files on the $(OBJ_DIR) directory.
clean:
	rm $(OBJ_DIR)/*.o
# Deletes all existing object files, recompiles and link them to make the final executable.
fresh: clean all
# Tries running the compiled executable if it's name wasn't changed
run:
	./$(OUTPUT)

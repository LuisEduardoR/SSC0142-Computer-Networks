FILES = ./src/main.cpp ./src/client/client.cpp ./src/server/server.cpp ./src/messaging/messaging.cpp
OUTPUT = trabalho-redes

all:

	g++ -pthread -o $(OUTPUT) $(FILES)

run:

	./$(OUTPUT)

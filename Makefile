FILES = ./src/main.c ./src/client/client.c ./src/server/server.c ./src/messaging/messaging.c
OUTPUT = trabalho-redes

all:

	gcc -o $(OUTPUT) $(FILES)

run:

	./$(OUTPUT)

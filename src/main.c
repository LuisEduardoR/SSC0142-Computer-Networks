// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client/client.h"
# include "server/server.h"

# include <stdio.h>
# include <stdlib.h>

# include <string.h>

// Default values.
# define DEFAULT_ADDR "127.0.0.1"
# define DEFAULT_PORT 9002

// Help texts.
# define HELP_NO_PARAMETERS "\nusage: ./trabalho-redes [parameters]\n\nFor a list of parameters type \"./trabalho-redes --help\"\n"
# define HELP_FULL "\nusage: ./trabalho-redes PARAMETERS\n\nYou can choose to connect as a client or as a server.\n\n\tTo connect as a client use:\n\t\t./trabalho-redes client (For default IP address and port)\n\t\t\tor\n\t\t./trabalho-redes client [IP adress] [port]\n\n\tTo connect as a server use:\n\t\t./trabalho-redes server (For default port)\n\t\t\tor\n\t\t./trabalho-redes server [port]\n"
# define HELP_CLIENT "\nusage:\n./trabalho-redes client (For default IP address and port)\n\tor\n./trabalho-redes client [IP adress] [port]\n"
# define HELP_SERVER "\nusage:\n./trabalho-redes server (For default port)\n\tor\n\t\t./trabalho-redes server [port]\n"

// Types of program instances.
enum TYPE { CLIENT, SERVER };

// Functions to handle the different types of instances this program can have.
// Declared bellow the main function.
int handle_client(char server_addr[16], int server_port);
int handle_server(int server_port);


// Program main function.
int main(int argc, char* argv[])
{

    // Displays help text, if wrong number of parameters was passed.
    if(argc < 2) {
        printf("%s\n", HELP_NO_PARAMETERS);
        return 0;
    }

    // Displays help text, if asked to.
    if(strcmp(argv[1],"--help") == 0) {
        printf("%s\n", HELP_FULL);
        return 0;
    }

    // Checks for the type to be used for this process instance.
    enum TYPE instance_type;
    if(strcmp(argv[1], "client") == 0) {
        instance_type = CLIENT;
    } else if(strcmp(argv[1], "server") == 0) {
        instance_type = SERVER;
    } else {
        printf("%s\n", HELP_FULL);
        return 0;
    }

    if(instance_type == CLIENT) { // Runs for the client.

        // Stores the IP address and port of ther server to connect.
        char server_addr[16];
        int server_port;

        // Checks for the client parameters.
        if(argc == 2) { // If no additional parameters were provided use the default ones.

            strcpy(server_addr, DEFAULT_ADDR);
            server_port = DEFAULT_PORT;

        } else if (argc == 4) { // If an IP address and a port were provided use them instead.

            // Checks if the IP address is valid.
            if(strlen(argv[2]) > strlen("xxx.xxx.xxx.xxx")) {

                fprintf(stderr, "\nInvalid IP adress lenght!\n\n");
                return -1;
            }

            // Uses the provided parameters.
            strcpy(server_addr, argv[2]);
            server_port = atoi(argv[3]);

        } else { // Displays help text if the number of parameters is invalid.
            printf("%s\n", HELP_CLIENT);
            return 0;
        }

        // Calls a function to handle the client.
        return handle_client(server_addr, server_port);

    } else if(instance_type == SERVER) { // Handles the server.

        // Stores the port where the server will be hosted.
        int server_port;

        // Checks for the client parameters.
        if(argc == 2) { // If no additional parameters were provided use the default ones.

            server_port = DEFAULT_PORT;

        } else if (argc == 3) { // If a port is provided use it instead.

            server_port = atoi(argv[2]);

        } else { // Displays help text if the number of parameters is invalid.
            printf("%s\n", HELP_SERVER);
            return 0;
        }

        // Calls a function to handle the server.
        handle_server(server_port);

    } else { // Don't do anything for other parameters.
        
        printf("\nInvalid parameter!\n\n");

    }  

    return 0;

}

int handle_client(char server_addr[16], int server_port) {

    // Creates the client.
    printf("\nCreating new client...\n");
    client *c = client_create();
    // Checks for errors creating the client.
    if(c == NULL) {
        fprintf(stderr, "Error creating new client! (-1)\n\n");
        return -1;
    }
    printf("Client created successfully!\n");

    // Connects to the server.
    printf("\nAttempting connection to server (%s:%d)...\n", server_addr, server_port);
    int cnct_status = client_connect(c, server_addr, server_port);
    // Checks for errors.
    if(cnct_status < 0) {
        fprintf(stderr, "Error connecting to remote socket! (%d)\n\n", cnct_status);
        client_delete(&c);
        return cnct_status;
    }
    printf("Connected to the server successfully!\n");

    // Receives data from server.
    char response_buffer[4096];
    client_receive_data(c, response_buffer, sizeof(response_buffer));
    // Print received data.
    printf("\nClient received from server: %s\n\n", response_buffer);

    char msg_buffer[4096] = "Hello server!";
    client_send_data(c, msg_buffer, sizeof(msg_buffer));
    printf("Message sent to server...\n\n");

    // Deletes the client.
    client_delete(&c);

    return 0;

}

int handle_server(int server_port) {

    // Creates the server.
    printf("\nCreating server at port %d...\n", server_port);
    server *s = server_create(server_port);
    // Checks for errors creating the server.
    if(s == NULL) {
        fprintf(stderr, "Error creating server! (-1)\n\n");
        return -1;
    }
    // Checks for errors.
    int svr_status = server_status(s);
    if(svr_status < 0) {
        fprintf(stderr, "Error connecting to socket! (%d)\n\n", svr_status);
        server_delete(&s);
        return svr_status;
    }
    printf("Server created successfully!\n");

    // Listens and accepts for client connections, gets back the socket for the connected client.
    printf("\nListening for clients...\n");
    server_listen(s);
    printf("Client connected!\n\n");

    // Sends a message to the client that connected.
    char msg_buffer[4096] = "Hello client!";
    server_send_data(s, msg_buffer, sizeof(msg_buffer));
    printf("Message sent to client...\n");

    // Receives data from a client.
    char response_buffer[4096];
    server_receive_data(s, response_buffer, sizeof(response_buffer));
    // Print received data.
    printf("\nServer received from client: %s\n\n", response_buffer);

    // Deletes the server.
    server_delete(&s);

    return 0;

}

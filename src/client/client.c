// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client.h"
# include "../messaging/messaging.h"

# include <stdlib.h>

# include <stdio.h>
# include <string.h>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

struct CLIENT
{
    int network_socket;
    struct sockaddr_in server_adress;
    int connection_status;
};

// Creates a new client with a network socket.
client *client_create() { 

    // Allocates memory for the new client.
    client *c = (client*)malloc(sizeof(client));
    if(c == NULL) // Verifies memory allocation error.
        return NULL;

    // Creates a TCP socket.
    c->network_socket = socket(AF_INET, SOCK_STREAM, 0);

    return c; 

}

// Tries connecting to a server and returns the connection status.
int client_connect(client *c, char *s_addr, int port_number){

    // Gets an adress for the socket.
    c->server_adress.sin_family = AF_INET;
    c->server_adress.sin_port = htons(port_number);
    inet_pton(AF_INET,  s_addr, &(c->server_adress.sin_addr));

    // Connects to the server.
    c->connection_status = connect(c->network_socket, (struct sockaddr *) &(c->server_adress), sizeof(c->server_adress));

    // Returns the status of the connection.
    return c->connection_status;

}

// Gets the client socket.
int client_get_socket(client *c) {

    return c->network_socket;

}

// Handles the client instance (control of the program is given to the client until it disconnects).
void client_handle(client* c) {

    char command_buffer[32];
    do {

        printf("\nEnter a command:\n\n\t/check\t-\tChecks for new messages\n\t/send\t-\tSend a message\n\t/quit\t-\tClose the connection and exit the program\n\n");
        scanf(" %31[^\n\r]", command_buffer); // Scan for commands.

        if(strcmp(command_buffer, "/check") == 0) {

            printf("\nChecking for new messages...\n");

            // Receives data from the server. A buffer with appropriate size is allocated and must be freed later!
            char *response_buffer = NULL;
            int buffer_size = 0;
            check_message(client_get_socket(c), &response_buffer, &buffer_size, MAX_BLOCK_SIZE);

            if(buffer_size > 0) {

                // Print received data.
                printf("\nNew message from server: %s\n", response_buffer);

            } else {

                printf("\nThe server was lost!\n");
                break;

            }

            // Frees the memory used by the buffer.
            free(response_buffer);

            continue;
        }

        if(strcmp(command_buffer, "/send") == 0) {

            printf("\nEnter the message:\n\n");

            // Receives the message to be sent to the server. A buffer with appropriate size is allocated and must be freed later!
            char *msg_buffer;
            scanf(" %m[^\n\r]", &msg_buffer);

            // Sends the message to the client.
            send_message(client_get_socket(c), msg_buffer, 1 + strlen(msg_buffer), MAX_BLOCK_SIZE);
            printf("\nMessage sent to server...\n");

            // Frees the memory used for the buffer.
            free(msg_buffer);

            continue;

        }

        if(strcmp(command_buffer, "/quit") == 0) {
            break;
        }

    } while (strcmp(command_buffer, "/quit") != 0);

}

// Deletes the client, closing the socket and freeing memory.
void client_delete(client **c) { 

    // Closes the socket.
    close((*c)->network_socket);

    free(*c); // Frees the struct.
    *c = NULL; // Sets the variable to NULL.

}


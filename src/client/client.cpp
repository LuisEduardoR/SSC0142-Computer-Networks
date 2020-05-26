// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client.h"

# include <stdlib.h>

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

// Deletes the client, closing the socket and freeing memory.
void client_delete(client **c) { 

    // Closes the socket.
    close((*c)->network_socket);

    free(*c); // Frees the struct.
    *c = NULL; // Sets the variable to NULL.

}


// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "server.h"

# include <stdlib.h>

# include <string.h>
# include <iostream>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

struct SERVER
{
    int server_socket;
    struct sockaddr_in server_adress;
    int server_status;

    std::vector<int> client_sockets;
};

// Creates a new server with a network socket and binds the socket.
server *server_create(int port_number) { 

    // Allocates memory for the new server.
    server *s = (server*)malloc(sizeof(server));
    if(s == NULL) // Verifies memory allocation error.
        return NULL;

    // Creates a TCP socket.
    s->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Gets an adress for the socket.
    s->server_adress.sin_family = AF_INET;
    s->server_adress.sin_port = htons(port_number);
    s->server_adress.sin_addr.s_addr = INADDR_ANY;

    // Binds the server to the socket.
    s->server_status = bind(s->server_socket, (struct sockaddr *) &(s->server_adress), sizeof(s->server_adress));

    return s; 

}

// Returns the server status, the server status is stored as the return of the bind function during server_create.
int server_status(server *s) {

    return s->server_status;

}

// Gets the sockets of the connected clients.
std::vector<int> server_get_client_sockets(server *s) {

    return s->client_sockets;

}

// Listen and accept incoming connections, return the socket from the incoming client.
int server_listen(server *s) {

    // Listens to the socket.
    listen(s->server_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    int new_client = accept(s->server_socket, NULL, NULL);
    s->client_sockets.push_back(new_client);

    for(int i = 0; i < s->client_sockets.size(); i++)
        printf("%i", s->client_sockets[i]);

    return new_client;
}

// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s) { 

    // Closes the socket.
    close((*s)->server_socket);

    free(*s); // Frees the struct.
    *s = NULL; // Sets the variable to NULL.

}
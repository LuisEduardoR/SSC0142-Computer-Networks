// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira

# include "server.h"

# include <stdlib.h>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

struct SERVER
{
    int network_socket;
    struct sockaddr_in server_adress;
    int server_status;

};

// Creates a new server with a network socket and binds the socket.
server *server_create(int port_number) { 

    // Allocates memory for the new server.
    server *s = malloc(sizeof(server));
    if(s == NULL) // Verifies memory allocation error.
        return NULL;

    // Creates a TCP socket.
    s->network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Gets an adress for the socket.
    s->server_adress.sin_family = AF_INET;
    s->server_adress.sin_port = htons(port_number);
    s->server_adress.sin_addr.s_addr = INADDR_ANY;

    // Binds the server to the socket.
    s->server_status = bind(s->network_socket, (struct sockaddr *) &(s->server_adress), sizeof(s->server_adress));

    return s; 

}

// Returns the server status, the server status is stored as the return of the bind function during server_create.
int server_status(server *s) {

    return s->server_status;

}

// Listen and accept incoming connections, return the socket from the incoming client.
int server_listen(server *s) {

    // Listens to the socket.
    listen(s->network_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    return accept(s->network_socket, NULL, NULL);

}

// Sends data to a client.
void server_send_data(int client_socket, char *response_buffer, int buffer_size) {

    send(client_socket, response_buffer, buffer_size, 0);

}

// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s) { 

    // Closes the socket.
    close((*s)->network_socket);

    free(*s); // Frees the struct.
    *s = NULL; // Sets the variable to NULL.

}
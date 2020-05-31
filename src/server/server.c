// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "server.h"
# include "../messaging/messaging.h"

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <pthread.h>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
typedef struct CLIENT_CONNECTION
{

    server *server_instance;
    int client_socket;

} client_connection;

// Struct for the server.
struct SERVER
{
    int server_socket;
    struct sockaddr_in server_adress;
    int server_status;

    client_connection **client_connections;
    int client_connections_count;

};

// Private function headers, will be declared bellow.
void *handle_client(void* client);
void *handle_connections(void *s);
void close_server();
client_connection *server_listen(server *s);

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

    // Initializes the client connections as empty.
    s->client_connections = NULL;
    s->client_connections_count = 0;

    return s; 

}

// Returns the server status, the server status is stored as the return of the bind function during server_create.
int server_status(server *s) {

    return s->server_status;

}

// Handles the server instance (control of the program is given to the server until it finishes).
void server_handle(server* s) {

    pthread_t connection_thread;
    pthread_create(&connection_thread, NULL, handle_connections, (void*)s);

    pthread_join(connection_thread, NULL);

}

void *handle_connections(void *s) {

    // Stores the server pointer.
    server *serv = (server*)s;

    while(1)
    {

        // Listens for new connection.
        client_connection *new_connection = server_listen(serv);

        // Adds the new connection to the list.
        serv->client_connections = (client_connection**)realloc(serv->client_connections, sizeof(client_connection*) * (serv->client_connections_count + 1));
        serv->client_connections[serv->client_connections_count] = new_connection;
        serv->client_connections_count++;

        // Handles the new connection client.
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void*)new_connection);

    }

}

void *handle_client(void* connect)
{

    // Sends an welcome message to the joining client.
    client_connection* client_connec = (client_connection*)(connect);
    send_message(client_connec->client_socket, "Welcome!", 9, MAX_BLOCK_SIZE);

    while(1) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        char *response_buffer = NULL;
        int buffer_size = 0;
        check_message(client_connec->client_socket, &response_buffer, &buffer_size, MAX_BLOCK_SIZE);
        
        // Redirects the message to the other clients.
        if(buffer_size > 0) {
            for(int i = 0; i < client_connec->server_instance->client_connections_count; i++) {
                if(client_connec->server_instance->client_connections[i] != client_connec)
                    send_message(client_connec->server_instance->client_connections[i]->client_socket, response_buffer, 1 + strlen(response_buffer), MAX_BLOCK_SIZE);
            }
        }

        // Frees the memory used by the buffer.
        free(response_buffer);
        
    }
}

// Listen and accept incoming connections, return the socket from the incoming client.
client_connection *server_listen(server *s) {

    // Listens to the socket.
    listen(s->server_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    int new_client_socket = accept(s->server_socket, NULL, NULL);

    // Creates a new connection object and assigns the socket.
    client_connection *new_connection = (client_connection*)malloc(sizeof(client_connection));
    new_connection->client_socket = new_client_socket;
    new_connection->server_instance = s;

    return new_connection;
}

// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s) { 

    // Closes the socket.
    close((*s)->server_socket);

    // Free memory used to store server connections.
    for(int i = 0; i < (*s)->client_connections_count; i++)
        free((*s)->client_connections[i]);
    free((*s)->client_connections);

    free(*s); // Frees the struct.
    *s = NULL; // Sets the variable to NULL.

}
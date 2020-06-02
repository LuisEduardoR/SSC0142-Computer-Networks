// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "server.h"
# include "../messaging/messaging.h"

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <errno.h>

# include <fcntl.h>
# include <signal.h>
# include <semaphore.h>
# include <pthread.h>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the server should be closed.
int CLOSE_SERVER_FLAG = 0;

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
typedef struct CLIENT_CONNECTION
{

    server *server_instance;
    int client_socket;
    pthread_t thread;

} client_connection;

// Struct for the server.
struct SERVER
{
    int server_socket;
    struct sockaddr_in server_adress;
    int server_status;

    sem_t updating_connections;
    client_connection **client_connections;
    int client_connections_count;

};

// Private function headers, will be declared bellow. ====================================================================
void *handle_client(void* client);
void *handle_connections(void *s);
void close_server();
client_connection *server_listen(server *s, int *status);
void remove_client(client_connection *connection);
// =======================================================================================================================

// Creates a new server with a network socket and binds the socket.
server *server_create(int port_number) { 

    // Allocates memory for the new server.
    server *s = (server*)malloc(sizeof(server));
    if(s == NULL) // Verifies memory allocation error.
        return NULL;

    // Creates a TCP socket.
    s->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Sets the socket to be non-blocking.
    int flags = fcntl(s->server_socket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(s->server_socket, F_SETFL, flags);

    // Gets an adress for the socket.
    s->server_adress.sin_family = AF_INET;
    s->server_adress.sin_port = htons(port_number);
    s->server_adress.sin_addr.s_addr = INADDR_ANY;

    // Binds the server to the socket.
    s->server_status = bind(s->server_socket, (struct sockaddr *) &(s->server_adress), sizeof(s->server_adress));

    // Initializes the semaphore used to protect the connections array.
    sem_init(&(s->updating_connections), 0, 1);

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

    // Sets the server to be closed when CTRL+C is pressed.
    signal(SIGINT, close_server);

    // Creates a thread to handle new connections.
    pthread_t connection_thread;
    pthread_create(&connection_thread, NULL, handle_connections, (void*)s);

    pthread_join(connection_thread, NULL);

}

void *handle_connections(void *s) {

    // Stores the server pointer.
    server *serv = (server*)s;

    while(!CLOSE_SERVER_FLAG)
    {

        // Listens for new connection.
        int status = 0;
        client_connection *new_connection = server_listen(serv, &status);

        if(new_connection == NULL) {

            if(status != 1) // If status is 1, no new connection is currently avaliable, otherwise an error ocurred.
                fprintf(stderr, "Unidentified connection error!\n");

        } else { // Initializes the new connection.

            // Handles creating the new connection and adding it to the array in a thread safe way. ----------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            sem_wait(&(serv->updating_connections));

            // ENTER CRITICAL REGION =======================================

            fprintf(stderr, "Client just connected with socket %d!\n", new_connection->client_socket);

            // Adds the new connection to the list, modifying the list can cause problems if some client handler is reading it at the same time, thus a semaphore is used.
            serv->client_connections = (client_connection**)realloc(serv->client_connections, sizeof(client_connection*) * (serv->client_connections_count + 1));
            serv->client_connections[serv->client_connections_count] = new_connection;
            serv->client_connections_count++;

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            sem_post(&(serv->updating_connections));

            // --------------------------------------------------------------------------------------------------------------------------------------------------

            // Creates a new thread to handle the connection.
            pthread_create(&(new_connection->thread), NULL, handle_client, (void*)new_connection);

        }

    }

}

void *handle_client(void* connect)
{

    // Gets a reference to the client handled by this thread.
    client_connection* client_connect = (client_connection*)(connect);

    // Sends a welcome message to the client joining the chat.
    char welcome_msg_buffer[53];
    sprintf(welcome_msg_buffer, "server: Welcome %d!", client_connect->client_socket);
    send_message(client_connect->client_socket, welcome_msg_buffer, strlen(welcome_msg_buffer) + 1);

    while(!CLOSE_SERVER_FLAG) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        char *response_buffer = NULL;
        int buffer_size = 0;
        check_message(client_connect->client_socket, &status, &response_buffer, &buffer_size);
        
        // Redirects the message to the other clients.
        if(status == 0) { // A message was received and must be redirected.

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            sem_wait(&(client_connect->server_instance->updating_connections));

            // ENTER CRITICAL REGION =======================================

            if(strcmp(response_buffer, "/ping") == 0) { // Detects the ping command.

                // Sends pong back to the client.
                send_message(client_connect->client_socket, "server: pong", 13);

            } else { // I fno command is detected, it's a regular message that needs to be sent to others.

                // Adds the socket from the client who sent the message to it's start.
                char nickname_buffer[53];
                sprintf(nickname_buffer, "%d: ", client_connect->client_socket);

                // Creates a new buffer to store the nickname and the message.
                int total_send_size = strlen(nickname_buffer) + strlen(response_buffer) + 1;
                char *sending_buffer = malloc(sizeof(char) * total_send_size);

                // Constructs the message to be sent.
                strcpy(sending_buffer, nickname_buffer);
                strcpy(sending_buffer + strlen(nickname_buffer), response_buffer);

                // Sends the message to all the other clients. Reading this list could cause problems if a new connection is being added or removed, thus a semaphore is used.
                for(int i = 0; i < client_connect->server_instance->client_connections_count; i++) {
                    if(client_connect->server_instance->client_connections[i] != client_connect)
                        send_message(client_connect->server_instance->client_connections[i]->client_socket, sending_buffer, total_send_size);
                }

                // Frees the buffer used for the sent message.
                free(sending_buffer);

            }

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            sem_post(&(client_connect->server_instance->updating_connections));

        } else if(status == 1) { // No new messages from this client.
            // If there are no messages nothing is done.
        } else if(status == -1) { // The client has disconnected.
            break; // Exits the thread.
        } else { // An error has happened.
            fprintf(stderr, "ERROR %d!\n", status);
        }   

        // Frees the memory used by the buffers.
        free(response_buffer);
        
    }

    // Disconnects the client before exiting this thread.
    remove_client(client_connect); 

}

// Listen and accept incoming connections, return the socket from the incoming client.
client_connection *server_listen(server *s, int *status) {

    // Listens to the socket.
    listen(s->server_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    int new_client_socket = accept(s->server_socket, NULL, NULL);

    // If no connection happened, checks why.
    if(new_client_socket == -1) {

        if(errno == EAGAIN || errno == EWOULDBLOCK) { // No connection is avaliable.
            *status = 1;
        } else { // Error.
            *status = -1;
        }

        return NULL;
    }

    // Creates a new connection object and assigns the socket.
    client_connection *new_connection = (client_connection*)malloc(sizeof(client_connection));
    new_connection->client_socket = new_client_socket;
    new_connection->server_instance = s;

    // Marks that there has been a new connection.
    *status = 0;

    return new_connection;
}

// Removes a client that has disconnected from the server.
void remove_client(client_connection *connection) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    sem_wait(&(connection->server_instance->updating_connections));

    // ENTER CRITICAL REGION =======================================

    fprintf(stderr, "Client with socket %d disconnected!\n", connection->client_socket);

    // Finds the connection that 
    int found_connection = 0;
    for(int i = 0; i < connection->server_instance->client_connections_count; i++) {

        if(found_connection) { // If the connection was found removes it,, and shifts the pointers after it.
            connection->server_instance->client_connections[i-1] = connection->server_instance->client_connections[i];
        } else if(connection->server_instance->client_connections[i] == connection) {
            found_connection = 1;
        }

    }

    // Re-sizes the connection list.
    connection->server_instance->client_connections_count--;
    connection->server_instance->client_connections = (client_connection**)realloc(connection->server_instance->client_connections, sizeof(client_connection*) * (connection->server_instance->client_connections_count));

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    sem_post(&(connection->server_instance->updating_connections));

    // Frees the current connection.
    free(connection);

}

// Sets the flag to indicate the server should be closed.
void close_server() { CLOSE_SERVER_FLAG = 1; }

// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s) { 

    // Closes the socket.
    close((*s)->server_socket);

    // Free memory used to store server connections.
    for(int i = 0; i < (*s)->client_connections_count; i++)
        free((*s)->client_connections[i]);
    free((*s)->client_connections);

    // Destroys the semaphores after threy are not needed anymore.
    sem_destroy(&((*s)->updating_connections));

    free(*s); // Frees the struct.
    *s = NULL; // Sets the variable to NULL.

}
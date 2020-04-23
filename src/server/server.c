// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "server.h"

# include <stdlib.h>

# include <string.h>

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

    int client_socket;

};

// Creates a new server with a network socket and binds the socket.
server *server_create(int port_number) { 

    // Allocates memory for the new server.
    server *s = malloc(sizeof(server));
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

// Listen and accept incoming connections, return the socket from the incoming client.
int server_listen(server *s) {

    // Listens to the socket.
    listen(s->server_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    s->client_socket = accept(s->server_socket, NULL, NULL);

}

// Sends data to a client.
void server_send_data(server *s, char *msg_buffer, int buffer_size, int max_block_size) {

    // Breaks the message into blocks of a maximum size.
    int sent = 0;
    while (sent < buffer_size)
    {

        // Calculates how much data to send in this block.
        int bytes_to_send;
        if(buffer_size - sent < max_block_size) {
            bytes_to_send = buffer_size - sent;
        } else {
            bytes_to_send = max_block_size;
        }

        // Sends the data.
        sent += send(s->client_socket, msg_buffer + sent, bytes_to_send, 0);
    }
}

// Tries receiving data from client and storing it on a buffer.
void server_receive_data(server *s, char **response_buffer, int *buffer_size, int max_block_size) {

    char *temp_buffer = malloc(max_block_size * sizeof(char));
    int bytes_received = 0;
    while (1)
    {

        // Tries receiving data.        
        int received_now = recv(s->client_socket, temp_buffer, max_block_size, 0);

        // Handles no data received.
        if(received_now == -1 || received_now == 0) {

            // If no data has been received before, them there's no data to be received.
            if(bytes_received == 0) {

                *response_buffer = NULL;
                *buffer_size = 0;
                break;

            }

        }

        // Realocates the final buffer.
        *response_buffer = realloc(*response_buffer, (bytes_received + received_now) * sizeof(char));

        // Copies the data to the permanent buffer.
        memcpy((*response_buffer) + bytes_received, temp_buffer, received_now);
        // Adds the new bytes to the buffer size.
        bytes_received += received_now;

        // Checks if the message has been received to it's end.
        if(received_now < max_block_size || (received_now == max_block_size && temp_buffer[max_block_size - 1] == '\0')) { 
            *buffer_size = bytes_received;
            break;
        }

    }
    
    // Frees the temporary buffer.
    free(temp_buffer);    

}

// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s) { 

    // Closes the socket.
    close((*s)->server_socket);

    free(*s); // Frees the struct.
    *s = NULL; // Sets the variable to NULL.

}
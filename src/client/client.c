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
    client *c = malloc(sizeof(client));
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

// Tries receiving data from server and storing it on a buffer.
void client_receive_data(client *c, char **response_buffer, int *buffer_size, int max_block_size) {

    char *temp_buffer = malloc(max_block_size * sizeof(char));
    int bytes_received = 0;
    while (1)
    {

        // Tries receiving data.        
        int received_now = recv(c->network_socket, temp_buffer, max_block_size, 0);

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

// Sends data to server.
void client_send_data(client *c, char *msg_buffer, int buffer_size, int max_block_size) {

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
        sent += send(c->network_socket, msg_buffer + sent, bytes_to_send, 0);
    }    

}

// Deletes the client, closing the socket and freeing memory.
void client_delete(client **c) { 

    // Closes the socket.
    close((*c)->network_socket);

    free(*c); // Frees the struct.
    *c = NULL; // Sets the variable to NULL.

}


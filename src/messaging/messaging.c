# include "messaging.h"

# include <stdlib.h>

# include <string.h>

# include <sys/types.h>
# include <sys/socket.h>

// Sends data to a socket.
void send_message(int socket, char *send_buffer, int buffer_size, int max_block_size) {

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
        sent += send(socket, send_buffer + sent, bytes_to_send, 0);
    }    

}

// Tries receiving data from a socket and storing it on a buffer.
void check_message(int socket, char **response_buffer, int *buffer_size, int max_block_size) {

    char *temp_buffer = malloc(max_block_size * sizeof(char));
    int bytes_received = 0;
    while (1)
    {

        // Tries receiving data.        
        int received_now = recv(socket, temp_buffer, max_block_size, 0);

        // Handles no data received.
        if(received_now < 1) {

            // If recv returns 0 it means the connection was closed.
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
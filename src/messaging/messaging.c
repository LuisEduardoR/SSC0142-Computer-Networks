# include "messaging.h"

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <errno.h>

# include <fcntl.h>

# include <sys/types.h>
# include <sys/socket.h>

// Sends data to a socket.
void send_message(int socket, char *send_buffer, int buffer_size) {

    // Breaks the message into blocks of a maximum size.
    int sent = 0;
    while (sent < buffer_size)
    {

        // Calculates how much data to send in this block.
        int bytes_to_send;
        if(buffer_size - sent < MAX_BLOCK_SIZE) {
            bytes_to_send = buffer_size - sent;
        } else {
            bytes_to_send = MAX_BLOCK_SIZE;
        }

        // Sends the data.
        sent += send(socket, send_buffer + sent, bytes_to_send, 0);
    }    

}

// Tries receiving data from a socket and storing it on a buffer.
void check_message(int socket, int *status, char **response_buffer, int *buffer_size) {

    // Ensures the socket is set to non-blocking.
    int flags = fcntl(socket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(socket, F_SETFL, flags);

    // Receives the message.
    char temp_buffer[MAX_BLOCK_SIZE];
    int bytes_received = 0;
    while (1)
    {

        // Tries receiving data.        
        int received_now = recv(socket, temp_buffer, MAX_BLOCK_SIZE, 0);

        // Handles no data received.
        if(received_now == 0) { // The server or client has disconnected in a ordenerly way.

                *status = -1;
                *response_buffer = NULL;
                *buffer_size = 0;
                break;

        } else if (received_now < 0) {

            if(errno == EAGAIN || errno == EWOULDBLOCK) { 
            
                if(bytes_received == 0) { // No new message.
                    *status = 1;
                    *response_buffer = NULL;
                    *buffer_size = 0;
                    break;
                } else { // Continues waiting for the message.
                    continue;   
                }
                
                
            } else { // Error.
                *status = -1;
                *response_buffer = NULL;
                *buffer_size = 0;
                break;
            }
            
        }

        // Realocates the final buffer.
        *response_buffer = (char*)realloc(*response_buffer, (bytes_received + received_now) * sizeof(char));

        // Copies the data to the permanent buffer.
        memcpy((*response_buffer) + bytes_received, temp_buffer, received_now);
        // Adds the new bytes to the buffer size.
        bytes_received += received_now;

        // Checks if the message has been received to it's end.
        if(received_now < MAX_BLOCK_SIZE || (received_now == MAX_BLOCK_SIZE && temp_buffer[MAX_BLOCK_SIZE - 1] == '\0')) { 
            *buffer_size = bytes_received;
            break;
        }

    }

}
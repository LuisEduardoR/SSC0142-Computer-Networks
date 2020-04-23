// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef MESSAGING_H
# define MESSAGING_H

// Sends data to a socket.
void send_message(int socket, char *send_buffer, int buffer_size, int max_block_size);
// Tries receiving data from a socket and storing it on a buffer.
void check_message(int socket, char **receive_buffer, int *buffer_size, int max_block_size);

# endif
// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef MESSAGING_H
# define MESSAGING_H

# include <string>

// The maximum number of bytes that can be sent or received at once.
constexpr size_t max_block_size = 4096;

// The message used to acknowledge data was received.
constexpr char acknowledge_message[] = "/ack";

// Sends data to a socket.
void send_message(int socket, const std::string &message);
// Tries receiving data from a socket and storing it on a buffer.
std::string check_message(int socket, int *const status, int need_to_acknowledge);

# endif
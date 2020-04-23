// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CLIENT_H
# define CLIENT_H

typedef struct CLIENT client; 

// Creates a new client with a network socket.
client *client_create();
// Tries connecting to a server and returns the connection status.
int client_connect(client *c, char *s_addr, int port_number);
// Gets the client socket.
int client_get_socket(client *c);
// Deletes the client, closing the socket and freeing memory.
void client_delete(client **c);

# endif
// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# define BACKLOG_LEN 8

typedef struct SERVER server; 

// Creates a new server with a network socket and binds the socket.
server *server_create(int port_number);
// Returns the server status, the server status is stored as the return of the bind function during server_create.
int server_status(server *s);
// Handles the server instance (control of the program is given to the server until it finishes).
void server_handle(server* s);
// Deletes the server, closing the socket and freeing memory.
void server_delete(server **s);

# endif
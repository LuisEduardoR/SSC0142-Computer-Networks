// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client/client.h"
# include "server/server.h"

# include <stdio.h>

# define DEFAULT_PORT 9002
# define DEFAULT_ADDR "127.0.0.1"

int main(void)
{

    // Prompts if the user wants to be a client or a server.
    printf("\nDo you want to create a client[c] or a server[s]?\n[c/s]: ");
    char input;
    scanf("%c", &input);

    if(input == 'c' || input == 'C') { // Handles the client.

        // Creates the client.
        printf("\nCreating new client...\n");
        client *c = client_create();
        // Checks for errors creating the client.
        if(c == NULL) {
            fprintf(stderr, "Error creating new client! (-1)\n\n");
            return -1;
        }
        printf("Client created successfully!\n");

        // Connects to a client.
        printf("\nConnecting to server...\n");
        int cnct_status = client_connect(c, DEFAULT_ADDR, DEFAULT_PORT);
        // Checks for errors.
        if(cnct_status < 0) {
            fprintf(stderr, "Error connecting to remote socket! (%d)\n\n", cnct_status);
            client_delete(&c);
            return cnct_status;
        }
        printf("Connected to the server successfully!\n");

        // Receives data from server.
        char response_buffer[4096];
        client_receive_data(c, response_buffer, sizeof(response_buffer));
        // Print received data.
        printf("\nClient received from server: %s\n\n", response_buffer);

        char msg_buffer[4096] = "Hello server!";
        client_send_data(c, msg_buffer, sizeof(msg_buffer));
        printf("Message sent to server...\n\n");

        // Deletes the client.
        client_delete(&c);

    } else if(input == 's' || input == 'S') { // Handles the server.

        // Creates the server.
        printf("\nCreating new server...\n");
        server *s = server_create(DEFAULT_PORT);
        // Checks for errors creating the server.
        if(s == NULL) {
            fprintf(stderr, "Error creating new server! (-1)\n\n");
            return -1;
        }
        // Checks for errors.
        int svr_status = server_status(s);
        if(svr_status < 0) {
            fprintf(stderr, "Error connecting to remote socket! (%d)\n\n", svr_status);
            server_delete(&s);
            return svr_status;
        }
        printf("Server created successfully!\n");

        // Listens and accepts for client connections, gets back the socket for the connected client.
        printf("\nListening for clients...\n");
        server_listen(s);
        printf("Client connected!\n\n");

        // Sends a message to the client that connected.
        char msg_buffer[4096] = "Hello client!";
        server_send_data(s, msg_buffer, sizeof(msg_buffer));
        printf("Message sent to client...\n");

        // Receives data from a client.
        char response_buffer[4096];
        server_receive_data(s, response_buffer, sizeof(response_buffer));
        // Print received data.
        printf("\nServer received from client: %s\n\n", response_buffer);

        // Deletes the server.
        server_delete(&s);

    } else { // Don't do anything for other parameters.
        
        printf("\nInvalid parameter!\n\n");

    }  

    return 0;

}
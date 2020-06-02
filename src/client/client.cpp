// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client.hpp"
# include "../messaging/messaging.hpp"

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <errno.h>

# include <fcntl.h>
# include <csignal>
# include <semaphore.h>
# include <pthread.h>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the client should be closed.
int CLOSE_CLIENT_FLAG = 0;

struct CLIENT
{
    int network_socket;
    struct sockaddr_in server_adress;
    int connection_status;

    sem_t updating_messages;
    char **new_messages;
    int new_messages_count;

};

// Private function headers, will be declared bellow. ====================================================================
void *client_check_message(void* current_client);
void show_new_messages(client *c);
// =======================================================================================================================

// Used to make the client don't close on a CTRL + C;
void ignore_sigint(int signal_num) {

    // Prints a message and resets the signal.
    printf("\nUse the /quit command or input EOF (CTRL+D) to exit!\n");
    std::signal(SIGINT, ignore_sigint);

}

// Creates a new client with a network socket.
client *client_create() { 

    // Allocates memory for the new client.
    client *c = (client*)malloc(sizeof(client));
    if(c == NULL) // Verifies memory allocation error.
        return NULL;

    // Creates a TCP socket.
    c->network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initializes the semaphore used to protect the connections array.
    sem_init(&(c->updating_messages), 0, 1);

    // Initializes the client new messages as empty.
    c->new_messages = NULL;
    c->new_messages_count = 0;

    return c; 

}

// Tries connecting to a server and returns the connection status.
int client_connect(client *c, const char *s_addr, int port_number){

    // Gets an adress for the socket.
    c->server_adress.sin_family = AF_INET;
    c->server_adress.sin_port = htons(port_number);
    inet_pton(AF_INET,  s_addr, &(c->server_adress.sin_addr));

    // Connects to the server.
    c->connection_status = connect(c->network_socket, (struct sockaddr *) &(c->server_adress), sizeof(c->server_adress));

    // Returns the status of the connection.
    return c->connection_status;

}

// Gets the client socket.
int client_get_socket(client *c) {

    return c->network_socket;

}

// Handles the client instance (control of the program is given to the client until it disconnects).
void client_handle(client* c) {

    // Sets the client to ignore SIGINT displaying a mesage instead.
    std::signal(SIGINT, ignore_sigint);

    // Creates a thread to constantly check for new messages.
    pthread_t check_message_thread;
    pthread_create(&check_message_thread, NULL, client_check_message, (void*)c);

    char command_buffer[32];
    do {

        printf("\nEnter a command:\n\n");
        printf("\t/new\t-\tShows any new messages\n");
        printf("\t/send\t-\tSend a message\n");
        printf("\t/ping\t-\tThe server answers \"pong\"\n");
        printf("\t/quit\t-\tClose the connection and exit the program\n\n");
        if(scanf(" %31[^\n\r]", command_buffer) == EOF) { // Scan for commands, finishes the program on EOF.
            CLOSE_CLIENT_FLAG = 1;
            continue;
        }

        if(strcmp(command_buffer, "/new") == 0) {

            printf("\nChecking for new messages...\n\n");

            // Handles showing the new messages in a thread safe way. ------------------------------------------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            sem_wait(&(c->updating_messages));

            // ENTER CRITICAL REGION =======================================

            show_new_messages(c);

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            sem_post(&(c->updating_messages));

            // --------------------------------------------------------------------------------------------------------------------------------------------------

            continue;
        }

        if(strcmp(command_buffer, "/send") == 0) {

            printf("\nEnter the message:\n\n");

            // Receives the message to be sent to the server. A buffer with appropriate size is allocated and must be freed later!
            char *msg_buffer;
            scanf(" %m[^\n\r]", &msg_buffer);

            // Checks if the message is valid.
            if(msg_buffer[0] == '/') {
                printf("\nA message can't start with '/'!\n");
            } else {

                // Sends the message to the server to be redirected to the other clients.
                int status;
                send_message(client_get_socket(c), msg_buffer, 1 + strlen(msg_buffer));
                printf("\nMessage sent to server...\n");

            }

            // Frees the memory used for the buffer.
            free(msg_buffer);

            continue;

        }


        if(strcmp(command_buffer, "/ping") == 0) {

            // Sends the the /ping command to the client, use /new to see if "pong" was received.
            int status;
            const char *ping = "/ping";
            send_message(client_get_socket(c), ping, 6);
            printf("\nPing sent to server... Use /new to check for the response!\n");

            continue;

        }

        if(strcmp(command_buffer, "/quit") == 0) {
            CLOSE_CLIENT_FLAG = 1;
            continue;
        }

    } while (!CLOSE_CLIENT_FLAG);

    // Joins the new message thread before returning.
    pthread_join(check_message_thread, NULL);

}

// Handles checking the client messages on a separate thread.
void *client_check_message(void* current_client) {

    client *c = (client*)current_client;

    while (!CLOSE_CLIENT_FLAG)
    {    

        // Receives data from the server. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        char *response_buffer = NULL;
        int buffer_size = 0;
        check_message(client_get_socket(c), &status, 1, &response_buffer, &buffer_size);

        if(status == 0) {

            // Handles showing the new messages in a thread safe way. ------------------------------------------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            sem_wait(&(c->updating_messages));

            // Increases the size of the new messages array.
            c->new_messages = (char**)realloc(c->new_messages, sizeof(char*) * (c->new_messages_count + 1));

            // Transfers the response buffer to the new message array.
            c->new_messages[c->new_messages_count] = response_buffer;
            response_buffer = NULL;
            c->new_messages_count++;

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            sem_post(&(c->updating_messages));

            // --------------------------------------------------------------------------------------------------------------------------------------------------

            // Sends a message to the server, informing the message was received.
            // send_message(c->network_socket,"ACK", 4);

        } else if (status == 1) { // No new messages.
            // Do nothing.
        } else { // Server was lost.
            CLOSE_CLIENT_FLAG = 1; // Sets the client to close.
        }

        free(response_buffer); // Frees the memory used by the buffer if necessary.

    }

    return NULL;

}

// Shows client new messages.
void show_new_messages(client *c) {

    if(c->new_messages_count < 1) {

        printf("No new messages available!\n");

    } else {

        printf("You have new messages:\n\n");

        for(int i = 0; i < c->new_messages_count; i++) {

            printf("- %s\n", c->new_messages[i]); // Prints the message.
            free(c->new_messages[i]); // Frees the message memory.

        }

        // Frees the message array.
        free(c->new_messages);
        c->new_messages = NULL;

        // Updates the message count.
        c->new_messages_count = 0;

    }

}

// Deletes the client, closing the socket and freeing memory.
void client_delete(client **c) { 

    // Closes the socket.
    close((*c)->network_socket);

    // Frees any remaining messages.
    for(int i = 0; i < (*c)->new_messages_count; i++)
        free((*c)->new_messages[i]); // Frees the message memory.
    free((*c)->new_messages); // Frees the message array.

    free(*c); // Frees the struct.
    *c = NULL; // Sets the variable to NULL.

}


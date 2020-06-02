// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# include <vector>
# include <thread>
# include <mutex>
# include <semaphore.h>
# include <netinet/in.h>

# define BACKLOG_LEN 8 // Max connections backlog
# define MAX_RESENDING_ATTEMPS 5 // Amount of times the server will try resending a message.
# define ACKNOWLEDGE_WAIT_TIME 400 // Amount of time the server will wait before an attempt to send a message is failed.

// Headers for the classes declared bellow
class server;
class client_connection;

// Struct for the server.
class server
{
    public:

        int server_socket;
        struct sockaddr_in server_adress;
        int server_status;

        std::mutex updating_connections;
        std::vector<client_connection*> client_connections;
        std::vector<std::thread> connection_threads;

        server(int port_number);
        ~server();

        // Handles the server instance (control of the program is given to the server until it finishes).
        void handle();

        // Listen and accept incoming connections, return the socket from the incoming client.
        client_connection *listen_for_connections(int *status);


};

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class client_connection
{

    public:

        client_connection(int socket, server *server_instance);

        int alive;

        server *server_instance;
        
        int client_socket;
        pthread_t thread;

        std::mutex updating_received_message_status;
        int ack_received_message;

    private:

};

# endif
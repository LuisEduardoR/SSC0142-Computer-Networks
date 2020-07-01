// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# include <set>
# include <vector>
# include <thread>
# include <mutex>
# include <semaphore.h>
# include <netinet/in.h>

# define BACKLOG_LEN 8 // Max connections backlog
# define MAX_RESENDING_ATTEMPS 5 // Amount of times the server will try resending a message.
# define ACKNOWLEDGE_WAIT_TIME 400 // Amount of time the server will wait before an attempt to send a message is failed.

# define CLIENT_ROLE_IDLE -1 // Clients on the default (idle) channel.
# define CLIENT_ROLE_NORMAL 0 // Client has a normal role in a channel.
# define CLIENT_ROLE_ADMIN 1 // Client has an admin role in a channel.

// Headers for the classes declared bellow
class server;
class client_connection;
class channel;

// Struct for the server.
class server
{
    public:

        int server_socket;
        struct sockaddr_in server_adress;
        int server_status;

        server(int port_number);
        ~server();

        // Handles the server instance (control of the program is given to the server until it finishes).
        void handle();

        std::mutex updating_connections;
        std::vector<client_connection*> client_connections;
        std::vector<std::thread> connection_threads;

        // Listen and accept incoming connections, return the socket from the incoming client.
        client_connection *listen_for_connections(int *status);

        std::mutex updating_channels;
        std::vector<channel*> channels;

        // Creates a new channel on this server.
        bool create_channel(std::string name);


};

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class client_connection
{

    public:

        client_connection(int socket, server *server_instance);

        std::mutex updating_client_connection;

        int alive;

        std::string nickname;

        int channel;
        int role;

        server *server_instance;
        
        int client_socket;
        pthread_t thread;

        int ack_received_message;

    private:

};

// Struct for a server channel
class channel
{

    public:

        int index;
        std::string name;

        channel(int index, std::string name);

        std::mutex updating_members;
        std::set<client_connection *> members;

        bool add_client(client_connection *client);
        bool remove_client(client_connection *client);

    private:

};

# endif
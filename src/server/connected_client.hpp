// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CONNECTED_CLIENT_H
# define CONNECTED_CLIENT_H

# include "connected_client.hpp"

# include <set>
# include <vector>
# include <thread>
# include <mutex>
# include <semaphore.h>
# include <netinet/in.h>

# define MAX_RESENDING_ATTEMPS 5 // Amount of times the server will try resending a message to a connected client.
# define ACKNOWLEDGE_WAIT_TIME 400 // Amount of time the server will wait before an attempt to send a message toa conencted client fails.

# define CLIENT_ROLE_IDLE -1 // Clients on the default (idle) channel.
# define CLIENT_ROLE_NORMAL 0 // Client has a normal role in a channel.
# define CLIENT_ROLE_ADMIN 1 // Client has an admin role in a channel.

// Headers for classes in other files that will be used bellow.
class server;

// Used to pass information about the message that needs to be redirected to the threads.
class redirected_message
{

    public:

        redirected_message(int max_resending_attempts, std::string message);

        int attempts;
        std::string message;

    private:

};

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class connected_client
{

    public:

        connected_client(int socket, server *server_instance);

        // Used to lock this client for updates.
        std::mutex updating;

        // If this conenction should be killed, used when the server closes.
        bool kill;

        // Nickname for this connected client.
        std::string nickname;

        int channel;
        int role;

        server *server_instance;
        
        int client_socket;
        pthread_t thread;

        int ack_received_message;

        // Thread that handles the client connection to the server.
        void t_handle();

        // Used by the server to redirect a message to acertain client.
        void redirect_message(std::string message);
        // Used as a worker thread to redirect messages to a client and check if the client received the message.
        void t_redirect_message_worker(redirected_message *redirect);

    private:

};

# endif
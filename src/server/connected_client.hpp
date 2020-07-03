// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CONNECTED_CLIENT_H
# define CONNECTED_CLIENT_H

# include <set>

# include <mutex>
# include <atomic>

# include <netinet/in.h>

# include "connected_client.hpp"

# define MAX_NICKNAME_SIZE 50 // Max size of a connect client's nickname.
# define MAX_RESENDING_ATTEMPS 5 // Amount of times the server will try resending a message to a connected client.
# define ACKNOWLEDGE_WAIT_TIME 400 // Amount of time the server will wait before an attempt to send a message toa connected client fails.

# define CLIENT_DEAD -2 // Client is marked to be "killed".
# define CLIENT_NO_CHANNEL -1 // Client has no channel.
# define CLIENT_ROLE_NORMAL 0 // Client has a normal role in a channel.
# define CLIENT_ROLE_ADMIN 1 // Client has an admin role in a channel.

// Headers for classes in other files that will be used bellow.
class server;

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class connected_client
{

    public:

        // CONSTRUCTOR
        connected_client(int socket, struct sockaddr client_address, socklen_t addr_len, server *server_instance);

        // Used to lock this client for updates.
        std::mutex updating;

        // If this conenction should be killed, used when the server closes.
        std::atomic_bool atmc_kill;

        // Nickname for this connected client.
        std::string nickname;
        // Stores an instance to the server this client is connected to.
        server *server_instance;

        // This client's socket.
        int client_socket;
        // This client's address.
        struct sockaddr client_address;
        socklen_t addr_len;

        // Stores the value to check if messages where received and acknowledged.
        std::atomic_int32_t atmc_ack_received_message;

        // Threads =============================================================

        // Thread that handles the client connection to the server (used as a thread).
        void t_handle();

        // Used as a worker thread to redirect messages to a client and check if the client received the message (used as a thread).
        void t_redirect_message_worker(std::string *message);

        // Gets/sets ============================================================

        // Tries updating the player nickname (gets a lock).
        bool l_set_nickname(std::string nickname);

        // Changes the channel this client is connected to (gets a lock).
        bool l_set_channel(int channel, int role);
        // Returns the channel this client is conencted to (gets a lock).
        int l_get_channel();
        // Returns the role of this client on it's channel (gets a lock).
        int l_get_role();

        // Commands =============================================================

        // Send message on the current channel (gets a lock).
        bool l_send_to_channel(std::string message);

        // Tries changing the client nickname.
        bool change_nickname(std::string new_nickname);

        // Tries joining a server channel.
        bool join_channel(std::string channel_name);

    private:

        // Current channel for this client and his respective role.
        int current_channel, channel_role;

};

# endif
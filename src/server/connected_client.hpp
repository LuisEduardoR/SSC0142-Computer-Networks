// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CONNECTED_CLIENT_H
# define CONNECTED_CLIENT_H

# include "connected_client.hpp"

# include <set>

# include <thread>
# include <atomic>

# include <netinet/in.h>

# define MAX_NICKNAME_SIZE 50 // Max size of a connect client's nickname.
# define MAX_RESENDING_ATTEMPS 5 // Amount of times the server will try resending a message to a connected client.
# define ACKNOWLEDGE_WAIT_TIME 0.400 // Amount of time the server will wait before an attempt to send a message to a connected client fails (in seconds).

# define CLIENT_NO_CHANNEL -1 // Client has no channel.
# define CLIENT_ROLE_NORMAL 0 // Client has a normal role in his current channel.
# define CLIENT_ROLE_ADMIN 1 // Client has an admin role in his current channel.

// Headers for classes in other files that will be used bellow.
class server;

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class connected_client
{

    public:

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        connected_client(int socket, server *server_instance);
        ~connected_client();

        // ==============================================================================================================================================================
        // Atomics ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* If this conenction should be killed, used when disconnecting from server. */
        std::atomic_bool atmc_kill;

        /* Stores the value to check if messages where received and acknowledged. */
        std::atomic_int32_t atmc_ack_received_message;

        // ==============================================================================================================================================================
        // Statics ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns if a given nickname is a valid nickname. */
        static bool is_valid_nickname(std::string &nickname);

        // ==============================================================================================================================================================
        // Spawns/threads ===============================================================================================================================================
        // ==============================================================================================================================================================

        /* Spawns the thread to handle this client's connection. */
        void spawn_handle();

        /* Thread that handles the client connection to the server (used as a thread). */
        void t_handle();

        /* Spawns a thread to handle sending a message to this client. */
        void spawn_send_message_worker(std::string *message);

        /* Used as a worker thread to redirect messages to a client and check if the client received the message (used as a thread). */
        void t_send_message_worker(std::string *message);

        // ==============================================================================================================================================================
        // Getters/setters ==============================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns this client's nickname. */
        int get_socket();

        /* Returns this client's nickname. */
        std::string get_nickname();

        /* Tries updating the player nickname. */
        bool set_nickname(std::string nickname);

        /* Changes the channel this client is connected to. */
        void set_channel(int channel, int role);

        /* Returns the channel this client is connected to. */
        int get_channel();

        /* Returns the role of this client on it's channel. */
        int get_role();

        /* Returns the ip of this client as a string. */
        std::string get_ip();

    private:

        // ==============================================================================================================================================================
        // Variables=====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Stores an instance to the server this client is connected to. */
        server *server_instance;

        /* This client's socket. */
        int client_socket;

        /* Nickname for this connected client. */
        std::string nickname;

        /* Current channel for this client and his respective role. */
        int current_channel, channel_role;

        /* Stores the thread that handles this connection. */
        std::thread handle;

};

# endif
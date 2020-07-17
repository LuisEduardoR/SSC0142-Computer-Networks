// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CONNECTED_CLIENT_H
# define CONNECTED_CLIENT_H

# include "connected_client.hpp"

# include <set>
# include <queue>

# include <thread>
# include <mutex>
# include <atomic>

# include <netinet/in.h>

// Max size of a connect client's nickname.
constexpr size_t max_nickname_size = 50;
// Amount of times the server will try resending a message to a connected client.
constexpr unsigned max_resending_attempts = 5;
// Amount of time the server will wait before an attempt to send a message to a connected client fails (in seconds).
constexpr float acknowledge_wait_time = 0.400;

// Possible role for the connected client.
enum client_role { cr_No_channel, cr_Normal, cr_Admin };

// Headers for classes in other files that will be used bellow.
class server;

// Struct for the connection, so that we can pass it as an argument to the handle_client threads
class connected_client
{

    public:

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        connected_client(const int socket, server *const server_instance);
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
        static bool is_valid_nickname(const std::string &nickname);

        // ==============================================================================================================================================================
        // Spawns =======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Spawns the thread to handle this client's connection. */
        void spawn_handle();

        // ==============================================================================================================================================================
        // Messaging ====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Adds a new message to queue to be sent to this client. */
        void send(const std::string &message);

        // ==============================================================================================================================================================
        // Getters/setters ==============================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns this client's nickname. */
        int get_socket() const;

        /* Returns this client's nickname. */
        std::string get_nickname() const;

        /* Tries updating the player nickname. */
        bool set_nickname(const std::string &nickname);

        /* Changes the channel this client is connected to. */
        void set_channel(const std::string &channel_name, client_role role);

        /* Returns the channel this client is connected to. */
        std::string get_channel() const;

        /* Returns the role of this client on it's channel. */
        client_role get_role() const;

        /* Returns the ip of this client as a string. */
        std::string get_ip() const;

    private:

        // ==============================================================================================================================================================
        // Variables=====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Stores an instance to the server this client is connected to. */
        server *const server_instance;

        /* This client's socket. */
        const int client_socket;

        // Used to store messages that need to be send to this client.
        std::queue<std::string> message_queue;
        // Used to lock the message queue when reading or writing to it.
        std::mutex updating_message_queue;

        /* Nickname for this connected client. */
        std::string nickname;

        /* Current channel for this client and his respective role. */
        std::string current_channel;
        client_role channel_role;

        /* Stores the thread that handles listening for this clients conenction. */
        std::thread listening_handle;
        /* Stores the thread that handles seninding messages to this client. */
        std::thread sending_handle;

        // ==============================================================================================================================================================
        // Threads ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Thread that handles listening for client connection. */
        void t_handle_listening();

        /* Thread that handles sending messages to the client. */
        void t_handle_sending();

};

# endif
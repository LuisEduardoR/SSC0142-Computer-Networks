// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# include "channel.hpp"
# include "request.hpp"
# include "connected_client.hpp"

# include <map>
# include <queue>

# include <thread>
# include <mutex>

# include <arpa/inet.h>
# include <netinet/in.h>

# define BACKLOG_LEN 8 // Max connections backlog

// Headers for classes in other files that will be used bellow.
class channel;
class connected_client;
class redirect_message;

// Struct for the server.
class server
{
    public:

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        server(int port_number);
        ~server();

        // ==============================================================================================================================================================
        // Server =======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns the status of the server */
        int get_status();

        /* Handles the server instance (control of the thread is given to the server until it finishes). */
        void handle();

        // ==============================================================================================================================================================
        // Requests =====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Makes a request to the server, that will be added to the request queue and handled as soon as possible. (gets a lock to the request_queue during execution) */
        void make_request(connected_client *origin, const std::string &content);

    private:

        // ==============================================================================================================================================================
        // Variables ====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Used to store information about the server socket and address. */
        int server_socket;
        struct sockaddr_in server_address;

        /* Stores the status of the server */
        int server_status;

        /* Used to store new clients that just connected to the server, before they are transferred to the main list that's used for processing requests. */
        std::queue<connected_client*> new_clients;
        /* Used to lock the new clients list when reading or writing to it. */
        std::mutex updating_new_clients;

        // Used to store the clients connected to the server that are currently being listened to and who's requests are being processed.
        std::set<connected_client*> clients;

        // Used to store the server's current channels.
        std::map<std::string, channel> channels;
        // Used to store the name of channels that became empty and need to be removed.
        std::queue<std::string> empty_channels;

        // Used to store requests that need to be executed by the server.
        std::queue<request> request_queue;
        // Used to lock the request queue when reading or writing to it.
        std::mutex updating_request_queue;

        // ==============================================================================================================================================================
        // Client handling ==============================================================================================================================================
        // ==============================================================================================================================================================

        /* Separate thread to handle the connection of new clients */
        void t_handle_connections();
        
        /* Checks for changes in client connections. Adding or removing them if necessary. */
        void check_connections();

        /* Checks for channels that became empty and can be deleted. */
        void check_channels();

        /* Removes the client with the given socket from the server. */
        void kill_client(connected_client *connection);

        // ==============================================================================================================================================================
        // Creates/deletes channels =====================================================================================================================================
        // ==============================================================================================================================================================
        
        // Creates a new channel on this server.
        bool create_channel(const std::string &channel_name, int admin_socket);

        /* Deletes an empty channel on this server. */
        bool delete_channel(const std::string &channel_name);

        // ==============================================================================================================================================================
        // Getters ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns a reference to a client with a certain socket. */
        connected_client *get_client_ref(int socket);

        /* Returns a reference to a client with a certain nickname. */
        connected_client *get_client_ref(const std::string &nickname);

        /* Returns a reference to a channel with a certain name. */
        channel *get_channel_ref(const std::string &channel_name);

        // ==============================================================================================================================================================
        // Requests =====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Sends a message from a client to other clients on it's channel. */
        void send_request(connected_client *const origin, const std::string &message);

        /* Tries changing the nickname of a certain client. */
        void nickname_request(connected_client *const origin, const std::string &nickname);

        /* Tries joining a channel with a certain name as a certain client, tries creating the channel if it doesn't exist. */
        void join_request(connected_client *const origin, const std::string &channel_name);

        /* Tries kicking a client that must be in the same channel. */
        void kick_request(connected_client *const origin, const std::string &nickname);

        /* Tries mutting/unmutting a client that must be in the same channel and must not already be muted/unmuted. */
        void toggle_mute_request(connected_client *const origin, const std::string &nickname, bool muted);

        /* Tries finding and showing the IP of a cçient a player that must be in the same channel. */
        void whois_request(connected_client *const origin, const std::string &nickname);

};

# endif
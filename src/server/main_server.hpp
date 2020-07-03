// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# include <vector>
# include <queue>
# include <thread>
# include <mutex>

# include <arpa/inet.h>
# include <netinet/in.h>

# include "channel.hpp"
# include "request.hpp"
# include "connected_client.hpp"

# define BACKLOG_LEN 8 // Max connections backlog

// Headers for classes in other files that will be used bellow.
class channel;
class connected_client;
class redirect_message;

// Struct for the server.
class server
{
    public:

        // CONSTRUCTOR
        server(int port_number);

        // DESTRUCTOR
        ~server();

        // Handles the server instance (control of the program is given to the server until it finishes).
        void handle();

        // Used to mark that the server connections are being updated.
        std::mutex updating_connections;

        // Stores the client connections.
        std::vector<connected_client*> client_connections;

        // Used to mark that the server channels are being updated.
        std::mutex updating_channels;

        // Stores the server channels.
        std::vector<channel*> channels;

        // Used as a thread to check for connecting clients.
        void t_check_for_connections();

        // Creates a new channel on this server.
        bool create_channel(std::string name, connected_client *admin);

        // Deletes a new channel on this server.
        bool delete_channel(int index);

        // Removes a client from the server.
        void remove_client(connected_client *connection);

        // Gets a reference to a client with a certain nickname (gets lock).
        connected_client *l_get_client_by_name(std::string client_nickname);

        /* Returns the status of the server */
        int get_status();

        /* Handles the connection of new clients */
        void thread_handle_connections();
        /* Handles the execution of client requests. */
        void thread_handle_requests();
        
        // ==============================================================================================================================================================
        // ==============================================================================================================================================================
        // ==============================================================================================================================================================

        /* Adds a new client to the server. (gets a lock to the clients list during execution) */
        void add_client(int socket);

        /* Returns a reference to a client with a certain socket. (gets a lock to the clients list during execution) */
        connected_client *l_get_client_ref(int socket);

        /* Returns a reference to a client with a certain nickname. (gets a lock to the clients list during execution) */
        connected_client *l_get_client_ref(std::string &nickname);

        /* Removes the client with the given socket from the server.(gets a lock to the clients list during execution) */
        void remove_client(int socket);

        /* Makes a request to the server, that will be added to the request queue and handled as soon as possible. (gets a lock to the request_queue during execution) */
        void make_request(request new_request);

        // ==============================================================================================================================================================

    private:

        // Used to store information about the server socket and address.
        int server_socket;
        struct sockaddr_in server_address;
        // Stores the status of the server
        int server_status;

        // ==============================================================================================================================================================
        // ==============================================================================================================================================================
        // ==============================================================================================================================================================

        // Used to store the clients connected to the server.
        std::set<connected_client*> clients;
        // Used to lock the connected client list when reading or writing to it.
        std::mutex updating_clients;

        // Used to store requests that need to be executed by the server.
        std::queue<request> request_queue;
        // Used to lock the request queue when reading or writing to it.
        std::mutex updating_request_queue;

        // ==============================================================================================================================================================
        // Requests =====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Tries changing the nickname of a certain client */
        void nickname_request(connected_client *client, std::string &nickname);


};

# endif
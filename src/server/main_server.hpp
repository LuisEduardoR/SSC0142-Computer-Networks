// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef SERVER_H
# define SERVER_H

# include <vector>
# include <thread>
# include <mutex>

# include <arpa/inet.h>
# include <netinet/in.h>

# include "channel.hpp"
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

        int server_socket;
        struct sockaddr_in server_address;
        int server_status;

        server(int port_number);
        ~server();

        // Handles the server instance (control of the program is given to the server until it finishes).
        void handle();

        // Used to mark that the server connections are being updated.
        std::mutex updating_connections;

        // Stores the client connections.
        std::vector<connected_client*> client_connections;
        // Stores the client connection handles threads.
        std::vector<std::thread> connection_threads;

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

};

# endif
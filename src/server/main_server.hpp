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

        std::mutex updating_connections;
        std::vector<connected_client*> client_connections;
        std::vector<std::thread> connection_threads;

        std::mutex updating_channels;
        std::vector<channel*> channels;

        // Used as a thread to check for connecting clients.
        void t_check_for_connections();

        // Creates a new channel on this server.
        bool create_channel(std::string name, connected_client *admin);

        void remove_client(connected_client *connection);

};

# endif
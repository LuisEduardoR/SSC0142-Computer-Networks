// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CHANNEL_H
# define CHANNEL_H

# include "main_server.hpp"

# include <set>
# include <vector>
# include <thread>
# include <mutex>
# include <semaphore.h>
# include <netinet/in.h>

// Headers for classes in other files that will be used bellow.
class server;
class connected_client;

// Struct for a server channel
class channel
{

    public:

        std::mutex updating;

        // Stores an instance to the server this client is connected to.
        server *server_instance;

        int index;
        std::string name;

        channel(int index, std::string name, server *server_instance);

        std::set<connected_client*> members;

        bool add_client(connected_client *client);
        bool remove_client(connected_client *client);

        // Posts a message on the channel sending it to all members.
        void post_message(connected_client *sender, std::string message);

    private:

};

# endif
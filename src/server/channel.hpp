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
class connected_client;

// Struct for a server channel
class channel
{

    public:

        int index;
        std::string name;

        channel(int index, std::string name);

        std::mutex updating_members;
        std::set<connected_client*> members;

        bool add_client(connected_client *client);
        bool remove_client(connected_client *client);

    private:

};

# endif
// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CHANNEL_H
# define CHANNEL_H

# include <set>
# include <mutex>

# include "main_server.hpp"

# define MAX_CHANNEL_NAME_SIZE 200 // Max size of a channel name.

// Headers for classes in other files that will be used bellow.
class server;
class connected_client;

// Struct for a server channel
class channel
{

    public:

        // CONSTRUCTOR
        channel(int index, std::string name, server *server_instance);

        // Used to lock the channel for updates that need to be done safely.
        std::mutex updating;

        // Stores an instance to the server this client is connected to.
        server *server_instance;

        // Index of the channel on the server channel list.
        int index;
        // Name used to refer to this channel by clients.
        std::string name;

        // Stores the channel members.
        std::set<connected_client*> members;

        // Adds and removes clients to the members list.
        bool add_client(connected_client *client);
        bool remove_client(connected_client *client);

        // Stores the muted members.
        std::set<connected_client*> muted;

        // Mutes and unmutes clients on the channel.
        bool l_toggle_mute_client(connected_client *client, bool muted);

        // Posts a message on the channel sending it to all members.
        bool post_message(connected_client *sender, std::string message);

    private:

};

# endif
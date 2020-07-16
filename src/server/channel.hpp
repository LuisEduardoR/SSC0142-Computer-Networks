// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CHANNEL_H
# define CHANNEL_H

# include "main_server.hpp"

# include <string>

# include <set>
# include <vector>

# include <atomic>

# define MAX_CHANNEL_NAME_SIZE 200 // Max size of a channel name.

// Headers for classes in other files that will be used bellow.
class server;
class connected_client;

// Struct for a server channel
class channel
{

    public:

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        channel(int index, std::string name, server *server_instance);

        // ==============================================================================================================================================================
        // Statics ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns if a given name is a valid channel name. */
        static bool is_valid_channel_name(std::string &channel_name);

        // ==============================================================================================================================================================
        // Add/remove ===================================================================================================================================================
        // ==============================================================================================================================================================

        /* Adds and removes clients with the provided sockets to/from the members list. */
        bool add_member(int socket);
        bool remove_member(int socket);

        // ==============================================================================================================================================================
        // Member operations ============================================================================================================================================
        // ==============================================================================================================================================================

        /* Mutes and unmutes members of the channel. */
        bool toggle_mute_member(int socket, bool muted);

        /* Checks if a certain client is muted on the server. */
        bool is_muted(int socket);

        // ==============================================================================================================================================================
        // Getters ======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Gets this channel's index. */
        int get_index();

        /* Checks if a certain client is the admin of the server. */
        std::string get_name();

        /* Gets an array of this channel's members sockets (it needs to be deleted later), and stores it's size on r_size 
        if passed as something other than nullptr. */
        int *get_members(int *r_size);

    private:

        // ==============================================================================================================================================================
        // Variables=====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Stores an instance to the server that has this channel */
        server *server_instance;

        /* Index of the channel on the server channel list. */
        int index;

        /* Name used to refer to this channel by clients. */
        std::string name;

        /* Stores the channel members, stores the sockets of the clients. */
        std::set<int> members;

        /* Stores the muted members. */
        std::set<int> muted;

};

# endif
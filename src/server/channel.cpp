// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "channel.hpp"

# include <iostream>
# include <string>

# include <set>
# include <vector>

# include <atomic>

// ==============================================================================================================================================================
// Constructors/destructors =====================================================================================================================================
// ==============================================================================================================================================================

// Creates a channel with an index and a name, also passes an instance of the server.
channel::channel(int index, std::string name, server *server_instance) {

    this->index = index;
    this->name = name;
    this->server_instance = server_instance;

}

// ==============================================================================================================================================================
// Statics ======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns if a given name is a valid channel name. */
bool channel::is_valid_channel_name(std::string &channel_name) {

    // Checks if the channel name has an invalid size.
    if(channel_name.length() > MAX_CHANNEL_NAME_SIZE) // Checks for valid size.
        return false;

    // Checks for invalid stater characters.
    if(channel_name[0] != '&' && channel_name[0] != '#')
        return false;

    // Checks for invalid characters on the whole channel name.
    for(auto it = channel_name.begin(); it != channel_name.end(); it++) { 
        if(*it == ' ' || *it == 7 || *it == ',')
            return false;
    }

    // If nothing invalid was found the channel name is valid.
    return true;

}

// ==============================================================================================================================================================
// Add/remove ===================================================================================================================================================
// ==============================================================================================================================================================

/* Adds client with socket provided to the members list. */
bool channel::add_member(int socket) {

    /* Adds the new client socket to the server. */
    if(this->members.find(socket) == this->members.end()) { // Checks if the client is already on this channel.

        this->members.insert(socket); // Add to channel members.

        std::cerr << "Client with socket " << socket << " is now on channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;
        return true;

    }

    std::cerr << "Error adding client with socket " << socket << " to channel " << this-> name << ": client is already on the channel! ";
    return false; 

}

/* Removes client with socket provided from the members list. */
bool channel::remove_member(int socket) {

    /* Removes the client from the server. */
    auto iter = this->members.find(socket); // Tries getting an iterator to the client socket to be removed.
    if(iter != this->members.end()) { // Checks if the client is on the channel.

        this->members.erase(iter); // Add to channel members.

        // Now tries removing from the muted list.
        iter = this->members.find(socket); // Tries getting an iterator to the client socket being removed on the muted list.
        if(iter != this->members.end()) // Removes the client socket from the muted list if necessary.
            this->muted.erase(iter);

        std::cerr << "Client with socket " << socket << " left channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;
        return true;

    }
        
    std::cerr << "Error removing client with socket " << socket << " from channel " << this-> name << ": client is not on the channel! ";
    return false;

}

// ==============================================================================================================================================================
// Member operations ============================================================================================================================================
// ==============================================================================================================================================================

/* Mutes and unmutes members of the channel. */
bool channel::toggle_mute_member(int socket, bool muted) {

    // Tries getting an iterator to the client socket being muted/unmuted.
    auto iter = this->muted.find(socket);

    // Adds the client socket to the muted list if it's not currently there.
    if(muted && iter == this->muted.end()) {
        this->muted.insert(socket);
        return true;
    } 
    
    // Removes the client socket from the muted list if it's currently there.
    if(!muted && iter != this->muted.end()) { 
        this->muted.erase(iter);
        return true;
    }

    return false;

}


/* Checks if a certain client is muted on the server. */
bool channel::is_muted(int socket) { return (this->muted.find(socket) != this->muted.end()); }

// ==============================================================================================================================================================
// Getters ======================================================================================================================================================
// ==============================================================================================================================================================

/* Gets this channel's index. */
int channel::get_index() { return this->index; }

/* Checks if a certain client is the admin of the server. */
std::string channel::get_name() { return this->name; }

/* Gets an array of this channel's members sockets (it needs to be deleted later), and stores it's size on r_size 
if passed as something other than nullptr. */
int *channel::get_members(int *r_size) {

    // Stores the value to be returned.
    int *members = nullptr;

    // Checks if there are actually members on this channel (in theory there should be at least one: the admin).
    if(this->members.size() > 0) {
        // Allocates enough space on the array.
        members = new int[this->members.size()];
        // Copies the members to the array.
        int cur_index = 0;
        for(auto it = this->members.begin(); it != this->members.end(); it++) {
            members[cur_index] = *it;
            cur_index++;
        }
        // Stores the array size if necessary.
        if(r_size != nullptr)
            *r_size = this->members.size();
    }

    return members;

}

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

/* Creates a channel with a certain name. */
channel::channel(std::string name) { this->name = name; }

// ==============================================================================================================================================================
// Statics ======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns if a given name is a valid channel name. */
bool channel::is_valid_channel_name(const std::string &channel_name) {

    // Checks if the channel name has an invalid size.
    if(channel_name.length() > MAX_CHANNEL_NAME_SIZE) // Checks for valid size.
        return false;

    // Checks for invalid stater characters.
    if(channel_name[0] != '&' && channel_name[0] != '#')
        return false;

    // Checks for invalid characters on the whole channel name.
    for(auto iter = channel_name.begin(); iter != channel_name.end(); iter++) { 
        if(*iter == ' ' || *iter == 7 || *iter == ',')
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

        this->members.erase(iter); // Remvoes from channel members.

        // Now tries removing from the muted list.
        iter = this->muted.find(socket); // Tries getting an iterator to the client socket being removed on the muted list.
        if(iter != this->muted.end()) // Removes the client socket from the muted list if necessary.
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

/* Checks if the channel has no members. */
bool channel::is_empty() { return this->members.empty(); }

// ==============================================================================================================================================================
// Getters ======================================================================================================================================================
// ==============================================================================================================================================================

/* Checks if a certain client is the admin of the server. */
std::string channel::get_name() { return this->name; }

/* Gets an array of this channel's members sockets. */
std::vector<int> channel::get_members() {

    // Converts the members set to a vector and returns it.
    return std::vector<int>(this->members.begin(), this->members.end());

}

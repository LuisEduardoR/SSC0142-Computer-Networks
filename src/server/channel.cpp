// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "channel.hpp"

# include <iostream>
# include <string>

# include <mutex>

// Creates a channel with an index and a name.
channel::channel(int index, std::string name, server *server_instance) {

    this->index = index;
    this->name = name;
    this->server_instance = server_instance;

}

bool channel::add_client(connected_client *client) {

    bool success = false;

    int old_channel; // Stores the client old channel.

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================
    
    // Gets the client's old channel.
    old_channel = client->get_channel();

    // Gets the role for the client being added.
    int role;
    if(this->index > 0) { // For normal channels.
            
        if(this->members.size() == 1)
            role = CLIENT_ROLE_ADMIN; // The first user to join is an admin.
        else
            role = CLIENT_ROLE_NORMAL; // Other users are normal.

    } else {  // For the idle channel there's a special role.
        role = CLIENT_ROLE_IDLE;
    }

    // Sets the client new channel and role.
    success = client->set_channel(this->index, role);
    
    // Adds client to this channel's members and removes from the old if it has changed channels successfully.
    if(success) {

        if(old_channel >= 0) // Removes from old channel.
            this->server_instance->channels[old_channel]->remove_client(client);

        this->members.insert(client); // Add to new channel.

    }

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    if(success) {
        std::cerr << "Client with socket " << client->client_socket << " is now on channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;
        return true;
    }
    
    std::cerr << "Error moving client with socket " << client->client_socket << " tp channel " << this-> name << "! ";
    return false;   

}

bool channel::remove_client(connected_client *client) {

    bool success = false;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================

    // Sets the client channel to none (note that if the client exited the channel orderly it should be sent back to the idle channel).
    success = client->set_channel(CLIENT_NO_CHANNEL, CLIENT_NO_CHANNEL);
    // Adds client to the channel members if it has changed channels successfully.
    if(success) this->members.erase(client);   
    
    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    if(success) {
        std::cerr << "Client with socket " << client->client_socket << " left channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;
        return true;
    }

    std::cerr << "Error removing client with socket " << client->client_socket << " from channel " << this-> name << "! ";
    return false;

}
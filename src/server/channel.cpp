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
    this->name = '#' + name;
    this->server_instance = server_instance;

}

bool channel::add_client(connected_client *client) {

    bool success = false;

    // Gets the old client channel.
    int old_channel_index = client->l_get_channel();

    // If the client is already on this channel, does nothing.
    if(old_channel_index == this->index)
        return false;

    // Removes the client from his current channel if his already in one.
    if(old_channel_index >= 0) {

        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->server_instance->updating_channels.lock();

        // ENTER CRITICAL REGION =======================================
        this->server_instance->channels[old_channel_index]->remove_client(client);
        // EXIT CRITICAL REGION ========================================

        // Exits the critical region, and opens the semaphore.
        this->server_instance->updating_channels.unlock();
    }

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();

    // ENTER CRITICAL REGION =======================================

    // Sets the client new channel and role.
    success = client->l_set_channel(this->index, CLIENT_ROLE_NORMAL);

    // Adds client to this channel's members and removes from the old if it has changed channels successfully.
    if(success)
        this->members.insert(client); // Add to new channel.

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();

    if(success) {
        std::cerr << "Client with socket " << client->client_socket << " is now on channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;
        return true;
    }
    
    std::cerr << "Error moving client with socket " << client->client_socket << " to channel " << this-> name << "! ";
    return false;   

}

bool channel::remove_client(connected_client *client) {

    bool success = false;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();

    // ENTER CRITICAL REGION =======================================

    // Sets the client channel to none (note that if the client exited the channel orderly it should be sent back to the idle channel).
    success = client->l_set_channel(CLIENT_NO_CHANNEL, CLIENT_NO_CHANNEL);
    // Adds client to the channel members if it has changed channels successfully.
    if(success) this->members.erase(client);   
    
    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();

    if(success) {
        
        std::cerr << "Client with socket " << client->client_socket << " left channel " << this-> name << "! ";
        std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;

        // Deletes the channel if it's now empty.
        if(this->members.size() < 1) // !FIXME: channel not being deleted.
            this->server_instance->delete_channel(this->index);

        return true;
    }

    std::cerr << "Error removing client with socket " << client->client_socket << " from channel " << this-> name << "! ";
    return false;

}

// Posts a message on the channel sending it to all members.
bool channel::post_message(connected_client *sender, std::string message) {

    bool success = false;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();

    // ENTER CRITICAL REGION =======================================

    // Send the message if client isn't muted.
    if(this->muted.find(sender) == this->muted.end()) {

        for(auto it = this->members.begin(); it != this->members.end(); it++) {

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            (*it)->updating.lock();

            // ENTER CRITICAL REGION =======================================
            // Creates the worker thread.
            std::thread worker(&connected_client::t_redirect_message_worker, (*it), new std::string(this->name + " " + message));
            worker.detach();
            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            (*it)->updating.unlock();
        }

        // Mark that the message has being sent.
        success = true;

    }
    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();

    return success;

}
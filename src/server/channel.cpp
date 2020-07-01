// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "channel.hpp"

# include <iostream>
# include <string>

# include <mutex>

// Creates a channel with an index and a name.
channel::channel(int index, std::string name) {

    this->index = index;
    this->name = name;

}

bool channel::add_client(connected_client *client) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================

    // Adds client to the channel.
    this->members.insert(client);
    
    
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    client->updating.lock();

        // ENTER CRITICAL REGION =======================================

        client->channel = this->index;

        if(this->index > 0) { // For normal channels.
            
            if(this->members.size() == 1)
                client->role = CLIENT_ROLE_ADMIN; // The first user to join is an admin.
            else
                client->role = CLIENT_ROLE_NORMAL; // Other users are normal.

        } else {  // For the idle channel there's a special role.
            client->role = CLIENT_ROLE_IDLE;
        }

        // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    client->updating.unlock();

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    std::cerr << "Client with socket " << client->client_socket << " is now on channel " << this-> name << "! ";
    std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;

    return true;

}

bool channel::remove_client(connected_client *client) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================

    // Adds client to the channel.
    this->members.erase(client);

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    client->updating.lock();

        // ENTER CRITICAL REGION =======================================

        // Sets the client channel to none (note that if the client exited the channel orderly it should be sent back to the idle channel).
        client->channel = -1;

        // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    client->updating.unlock();
    

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    std::cerr << "Client with socket " << client->client_socket << " left channel " << this-> name << "! ";
    std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;

    return true;

}
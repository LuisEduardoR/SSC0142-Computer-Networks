// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "main_server.hpp"
# include "connected_client.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <fcntl.h>
# include <csignal>

# include <mutex>
# include <thread>

# include <sys/types.h>
# include <sys/socket.h>

#include <arpa/inet.h>

#include <unistd.h>

// CONSTRUCTOR
connected_client::connected_client(int socket, server *server_instance) {

    this->atmc_kill = false;
    this->client_socket = socket;

    this->server_instance = server_instance;
    this->atmc_ack_received_message = 0;

    // Initially the nickname comes from the client socket.
    this->nickname = "socket " + std::to_string(socket);

    // Initially all clients have no channel.
    this->current_channel = -1;

}

// ==============================================================================================================================================================
// Threads ======================================================================================================================================================
// ==============================================================================================================================================================

// Thread that handles the client connection to the server.
void connected_client::t_handle() {

    // Runs while the client is connected.
    while(!this->atmc_kill) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string received_message = check_message(this->client_socket, &status, 0);

        // Checks for the status of the received message.
        switch(status) { 

            case 0: // A message was received and must be treated. =================================================================

                // Detects that the client is confirming a received message.
                if(received_message.compare(ACKNOWLEDGE_MESSAGE) == 0) {

                    // Marks that the client has acknowledge a message.
                    this->atmc_ack_received_message--; 

                // Detects regular message that needs to be posted in the channel.
                } else if (received_message.substr(0,6).compare("/send ") == 0) {

                    // Sends a message to the channel.
                    this->l_send_to_channel(received_message.substr(6,received_message.length()));

                // Detects the ping command and sends a "pong" in return.
                } else if(received_message.substr(0,5).compare("/ping") == 0 && received_message.length() == std::string("/ping").length()) {

                    // Creates the worker thread to redirect (send) the pong message.
                    std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: pong"));
                    worker.detach();

                // Detects the nickname command, tries changing the client nickname and sends a message telling the results.
                } else if(received_message.substr(0,10).compare("/nickname ") == 0) { 

                    // Try changing the nickname to the one provided.
                    this->change_nickname(received_message.substr(10,received_message.length()));

                // Detects the join command, tries joining a channel and sends a message telling the results.
                } else if(received_message.substr(0,6).compare("/join ") == 0) { 

                    // Try joining the channel with the name provided.
                    this->join_channel(received_message.substr(6,received_message.length()));
                
                // Checks for the admin commands, if the client is an admin.
                } else if(this->l_get_role() == CLIENT_ROLE_ADMIN) {

                    if(received_message.substr(0,6).compare("/kick ") == 0) { 
                        // TODO: kick command.
                    } else if(received_message.substr(0,6).compare("/mute ") == 0) { 
                        // TODO: mute command.
                    } else if(received_message.substr(0,8).compare("/unmute ") == 0) { 
                        // TODO: unmute command.
                    } else if(received_message.substr(0,7).compare("/whois ") == 0) { 

                        // Tries printing the ip of a certain client.
                        this->whois_client(received_message.substr(7,received_message.length()));

                    }
                }
                
                break;

            case 1: // No new messages from this client. =========================================================================
                break; // If there are no messages nothing is done.

            case -1: // The client has disconnected. =============================================================================
                this->atmc_kill = true; // Kills the conencted client. 
                break;

            default: // An error has happened. ===================================================================================
                std::cerr << "ERROR " << status << "!" << std::endl;
                break;
        }        
    }

    // Disconnects the client before exiting this thread.
    this->server_instance->remove_client(this);

}

// Used as a worker thread to redirect messages to a client and check if the client received the message.
void connected_client::t_redirect_message_worker(std::string *message) {

    if(this->atmc_kill) // Checks if the client is still connected.
        return;

    // Marks that this clients needs to acknowledges that a message has being received.
    this->atmc_ack_received_message++;

    // Marks how many attempts are left for a client to receive and acknowledge this message.
    int attempts = MAX_RESENDING_ATTEMPS;

    // Attempt sending the message.
    int success = 0;
    while (attempts > 0) {

        if(this->atmc_kill) // Checks if the client is still connected.
            break;

        // Sends the message.
        send_message(this->client_socket, *message);

        // Sleeps the thread for the desired time to wait for a response.
        usleep(ACKNOWLEDGE_WAIT_TIME);

        // Marks that all messages were successfully sent.
        if(this->atmc_ack_received_message < 1)
            success = 1;

        // Breaks when the message was successfully sent.
        if(success)
            break;

        // If the message failed to be sent, decreases the number of attemps remaining.
        attempts--;

    }

    // Frees the message memory.
    delete message;

    // If the client could not confirm the message was received, shut it down.
    if(!success && !this->atmc_kill)
        shutdown(this->client_socket, 2);
    
}

// ==============================================================================================================================================================
// Gets/Sets ====================================================================================================================================================
// ==============================================================================================================================================================

// Tries updating the player nickname.
bool connected_client::l_set_nickname(std::string nickname) {

    if(this->atmc_kill) // Checks if the client is still connected.
        return false;

    // Checks if the nickname is valid.
    bool has_alphanum = false;
    if(nickname.length() > MAX_NICKNAME_SIZE) // Checks for valid size.
        return false;
    // Checks for valid characters, alphanumerical, spaces, parentesis and dashes.
    for(auto it = nickname.begin(); it != nickname.end(); it++) { 
        if(std::isalnum(*it)) // Marks if the character is alphanumerical
            has_alphanum = true;
        else if(!(*it == ' ' || *it == '_' || *it == '-' || *it == '(' || *it == ')')) // Otherwise it must be one of this symbols.
            return false;
    }
    if(!has_alphanum) // Ensures at least one character is alphanumerical.
        return false;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================
    this->nickname = nickname;
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
   
    return true;

}

// Changes the channel this client is connected to.
bool connected_client::l_set_channel(int channel, int role) {

    if(this->atmc_kill) // Checks if the client is still connected.
        return false;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================
    this->current_channel = channel;
    this->channel_role = role;    
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
    return true;

}

// Returns the channel this client is conencted to.
int connected_client::l_get_channel() {

    if(this->atmc_kill) // Checks if the client is still connected.
        return CLIENT_DEAD;

    int channel;
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================
    channel = this->current_channel;
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
    return channel;

}

// Returns the role of this client on it's channel.
int connected_client::l_get_role() {

    if(this->atmc_kill) // Checks if the client is still connected.
        return CLIENT_DEAD;

    int role;
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================
    role = this->channel_role;
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
    return role;

}

// Returns the ip of this client as a string (gets a lock).
std::string connected_client::l_get_ip() {

    if(this->atmc_kill) // Checks if the client is still connected.
        return nullptr;

    std::string ip;
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================

    // Gets the ip address for the this clients socket.
    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);
    if(!getsockname(this->client_socket, (struct sockaddr*)(&sa), &sa_len)) {

        // Gets the ip.
        char *ip_char = inet_ntoa(sa.sin_addr);
        // Converts to std::string.
        ip = std::string(ip_char);

    }

    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
    return ip;

}

// ==============================================================================================================================================================
// Commands =====================================================================================================================================================
// ==============================================================================================================================================================

// Send message on the current channel.
bool connected_client::l_send_to_channel(std::string message) {
    
    // Checks if the client is on a channel before sending message.
    if(this->l_get_channel() < 0) {
        std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: you need to join a channel before sending messages!"));
        worker.detach();
        return false;
    }

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();
    // ENTER CRITICAL REGION =======================================

    // Creates a string to be posted, in the format NICKNAME:MESSAGE.
    std::string sending_string = this->nickname + ": " + message;

    // Gets reference to the channel to send the message.
    channel *target_channel = nullptr;
    if(this->current_channel >= 0)
        target_channel = this->server_instance->channels[this->current_channel];

    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();
    
    // Checks if the client is in a valid channel.
    if (target_channel != nullptr) {

        // Post the message on the channel.
        bool success = target_channel->post_message(this, sending_string);

        if(!success) { // Sends a message to the client if he's muted and can't send messages.
            std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: you are muted and can't send messages on channel " + target_channel->name + "!"));
            worker.detach();
        }

        return success;

    } else { // Gives an error if no valid channel was found.
        std::cerr << "Error getting client channel!" << std::endl;
        return false;
    }

}

// Tries changing the client nickname.
bool connected_client::change_nickname(std::string new_nickname) {

    // Checks if the nickname doesn't exist on the server.
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->server_instance->updating_connections.lock();
    // ENTER CRITICAL REGION =======================================
    bool nickname_exists = false;
    for(auto it = this->server_instance->client_connections.begin(); it != this->server_instance->client_connections.end(); it++) {

        if((*it)->nickname.compare(new_nickname) == 0) {
            nickname_exists = true;
            // Sends a message to the client telling the nickname already exists.
            std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: nickname already exists!"));
            worker.detach();
            break;
        }

    }
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->server_instance->updating_connections.unlock();

    // If the nickname doesn't exist, tries setting it.
    bool success = false;
    if(!nickname_exists) {

        success = this->l_set_nickname(new_nickname); // Tries updating the nickname. 

        // Sends a message to the client telling the results.
        if(success) {
            std::cerr << "Client with socket " << this->client_socket << " changed his nickname to " + this->nickname + "." << std::endl;
            std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: your nickname was changed to " + new_nickname + "!"));
            worker.detach();
        } else {
            std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: invalid nickname!"));
            worker.detach();
        }
    }

    return success;

}

// Tries joining a server channel.
bool connected_client::join_channel(std::string channel_name) {

    // Searches for the target channel.
    channel *target_channel = nullptr;
    for(auto it = this->server_instance->channels.begin(); it != this->server_instance->channels.end(); it++) {
        if((*it)->name.compare('#' + channel_name) == 0) {
            target_channel = *it;
            break;
        }
    }

    // Stores if the client has had success joining the channel.
    bool success = false;

    // If no channel exists tries creating a channel with this client as admin.
    if(target_channel == nullptr) {

        // Creates the channel and adds the client as admin.
        success = this->server_instance->create_channel(channel_name, this);

        // Sends a message with the results.
        if(success) {
            
            // Sends a message to enable showing the admin commands.
            std::thread worker_enable_commands(&connected_client::t_redirect_message_worker, this, new std::string("/show_admin_commands"));
            worker_enable_commands.detach();

            // Sends a message with text to warn the client of the success.
            std::thread worker_warning(&connected_client::t_redirect_message_worker, this, new std::string("server: you're now on channel #" + channel_name + " as an admin!"));
            worker_warning.detach();

        } else {
            std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: the channel #" + channel_name + "doesn't exist and can't be created! (Invalid name)"));
            worker.detach();
        }

    } else { // If a channel exists tries joining it.

        // Tries changing the channel.
        success = target_channel->add_client(this);

        // Sends a message with the results.
        std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: you're now on channel #" + channel_name + "!"));
        worker.detach();

    }

    return success;

}

// Prints the IP of a client to the admin.
bool connected_client::whois_client(std::string client_name) {

    // Gets the target client.
    connected_client *target_client = this->server_instance->l_get_client_by_name(client_name);

    // If the client wasn't found returns.
    if(target_client == nullptr) {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: no client with nickname " + client_name + "!"));
        worker.detach();
        return false;
    }

    // Checks if the this client and the target are on the same channel.
    if(target_client->l_get_channel() == this->l_get_channel()) {

        // Sends a message with the results.
        std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: the ip for " + client_name + " is " + target_client->l_get_ip() + "."));
        worker.detach();

        return true;

    } else {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_redirect_message_worker, this, new std::string("server: the client needs to be in the same channel as you for this action!"));
        worker.detach();
        return false;
    }

    

}
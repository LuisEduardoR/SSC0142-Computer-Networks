// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "main_server.hpp"
# include "connected_client.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <set>

# include <thread>
# include <atomic>

# include <chrono>

# include <fcntl.h>
# include <csignal>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

# include <unistd.h>

// ==============================================================================================================================================================
// Constructors/destructors =====================================================================================================================================
// ==============================================================================================================================================================

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

connected_client::~connected_client() {

    // Joins the handle thread.
    this->handle.join();

    // Closes the socket.
    close(this->client_socket);

}

// ==============================================================================================================================================================
// Statics ======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns if a given nickname is a valid nickname. */
bool connected_client::is_valid_nickname(std::string &nickname) {

    // Checks if the nickname has an invalid size.
    if(nickname.empty() || nickname.length() > MAX_NICKNAME_SIZE) // Checks for valid size.
        return false;

    // Checks for invalid stater characters.
    if(nickname[0] == '&' || nickname[0] == '#')
        return false;
        
    // Checks for invalid characters on the whole nickname.
    for(auto it = nickname.begin(); it != nickname.end(); it++) { 
        if(*it == ' ' || *it == 7 || *it == ',')
            return false;
    }

    // If nothing invalid was found the nickname is valid.
    return true;

}

// ==============================================================================================================================================================
// Spawns/threads ===============================================================================================================================================
// ==============================================================================================================================================================

/* Spawns the thread to handle this client's connection. (Stores it to be joined later) */
void connected_client::spawn_handle() { this->handle = std::thread(&connected_client::t_handle, this); }

/* Spawns a thread to handle sending a message to this client. */
void connected_client::spawn_send_message_worker(std::string *message) {
    // Spawns the worker thread to send a message to this client and detach the thread so it doesn't needs to be joined.
    std::thread worker(&connected_client::t_send_message_worker, this, message);
    worker.detach();
}

// Thread that handles the client connection to the server.
void connected_client::t_handle() {

    // Runs while the client is connected.
    while(!this->atmc_kill) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string new_message = check_message(this->client_socket, &status, 0);

        // Checks for the status of the received message.
        switch(status) { 

            case 0: // A new message with a request was received and must be treated. ============================================

                // ! Checks for requests that can be handled immediately, some of those are really important to be done as soon as possible like /ack, others
                // ! like /ping are done this way simple because it's possible and the request is not worth enough to waste the server's time.
                if(new_message.compare(ACKNOWLEDGE_MESSAGE) == 0) // Marks that the client has acknowledge a message (done here to avoid delays on the queue).       
                    this->atmc_ack_received_message--;
                else if(new_message.compare("/ping") == 0) // Sends a "pong" back to the client (done here to avoid delays on the queue).       
                    this->spawn_send_message_worker(new std::string("server: pong"));
                else // If the request can't be handled here puts it on the request queue.
                    this->server_instance->make_request(this, new_message);
                break;

            case 1: // No new messages from this client. =========================================================================
                break; // If there are no messages nothing is done.

            case -1: // The client has disconnected. =============================================================================
                this->atmc_kill = true; // Kills the connected client. 
                break;

            default: // An error has happened. ===================================================================================
                std::cerr << "ERROR " << status << "!" << std::endl;
                break;
        }        
    }
}

// Used as a worker thread to redirect messages to a client and check if the client received the message.
void connected_client::t_send_message_worker(std::string *message) {

    // Marks how many attempts are left for a client to receive and acknowledge this message.
    int attempts = MAX_RESENDING_ATTEMPS;

    // Marks that this clients needs to acknowledges that a message has being received.
    this->atmc_ack_received_message++;

    // Used to measure time, when verifying if the message arrived and was acknowledged.
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> current_time;
    std::chrono::duration<double> diff;

    // Attempt sending the message while there's attempts left and the client is connected.
    bool success = false;
    while (!success && attempts > 0 && !this->atmc_kill) {

        // Gets the current time.
        current_time = std::chrono::steady_clock::now();
        // Calculates the time difference.
        diff = current_time - start_time;

        // Checks if it's time to ressend the message or if it's the first attempt..
        if(diff.count() > ACKNOWLEDGE_WAIT_TIME || attempts == MAX_RESENDING_ATTEMPS) {

            if(attempts < MAX_RESENDING_ATTEMPS) // If the message failed to be sent and this is a retry prints a message.
                std::cerr << "Client with socket " << std::to_string(this->client_socket) << " failed to acknowledge message! (" << std::to_string(attempts) << " remaining)" << std::endl;

            // Attempt to send the message.
            send_message(this->client_socket, *message);
            attempts--;

            // Gets the start time for this attempt.
            start_time = std::chrono::steady_clock::now();
        }

        // Marks if all messages were successfully sent.
        if(this->atmc_ack_received_message < 1)
            success = true;

    }

    // Frees the message memory.
    delete message;

    // If the client could not confirm the message was received, shut it down.
    if(!success && !this->atmc_kill)
        shutdown(this->client_socket, SHUT_RDWR);
    
}

// ==============================================================================================================================================================
// Getters/setters ==============================================================================================================================================
// ==============================================================================================================================================================

/* Returns this client's nickname (doesn't use a lock, because socket is supposed to never change after being 
assigned by the constructor). */
int connected_client::get_socket() {
    return this->client_socket;
}

/* Returns this client's nickname. */
std::string connected_client::get_nickname() {
    return this->nickname;
}

/* Tries updating the player nickname. */
bool connected_client::set_nickname(std::string nickname) {

    // Checks if the nickname is valid.
    if(!connected_client::is_valid_nickname(nickname))
        return false;

    // Sets the nickname and returns a success.
    this->nickname = nickname;
    return true;

}

/* Changes the channel this client is connected to. */
void connected_client::set_channel(int channel, int role) {
    this->current_channel = channel;
    this->channel_role = role;    
}

/* Returns the channel this client is connected to. */
int connected_client::get_channel() {
    return this->current_channel;
}

/* Returns the role of this client on it's channel. */
int connected_client::get_role() {
    return this->channel_role;
}

/* Returns the ip of this client as a string. */
std::string connected_client::get_ip() {
    
    // Gets the client network address information.
    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);
    if(!getpeername(this->client_socket, (struct sockaddr*)(&sa), &sa_len)) {
        // Gets the ip.
        char *ip_char = inet_ntoa(sa.sin_addr);
        // Converts to std::string and returns.
        return std::string(ip_char);
    }

    // Returns an empty string.
    return std::string();

}
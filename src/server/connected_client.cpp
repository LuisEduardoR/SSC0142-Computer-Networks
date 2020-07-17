// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "connected_client.hpp"

# include "main_server.hpp"
# include "../color.hpp"
# include "../messaging.hpp"

# include <iostream>
# include <string>

# include <set>
# include <queue>

# include <thread>
# include <mutex>
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

connected_client::connected_client(int socket, server *const server_instance) {

    this->atmc_kill = false;
    this->client_socket = socket;

    this->server_instance = server_instance;
    this->atmc_ack_received_message = 0;

    // Initially the nickname comes from the client socket.
    this->nickname = "socket " + std::to_string(socket);

    // Initially all clients have no channel.
    this->current_channel = "NONE";

}

connected_client::~connected_client() {

    // Joins the handler threads.
    this->listening_handle.join();
    this->sending_handle.join();

    // Closes the socket.
    close(this->client_socket);

}

// ==============================================================================================================================================================
// Statics ======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns if a given nickname is a valid nickname. */
bool connected_client::is_valid_nickname(const std::string &nickname) {

    // Checks if the nickname has an invalid size.
    if(nickname.empty() || nickname.length() > MAX_NICKNAME_SIZE) // Checks for valid size.
        return false;

    // Checks for invalid stater characters.
    if(nickname[0] == '&' || nickname[0] == '#')
        return false;
        
    // Checks for invalid characters on the whole nickname.
    for(auto iter = nickname.begin(); iter != nickname.end(); iter++) { 
        if(*iter == ' ' || *iter == 7 || *iter == ',')
            return false;
    }

    // If nothing invalid was found the nickname is valid.
    return true;

}

// ==============================================================================================================================================================
// Spawns/threads ===============================================================================================================================================
// ==============================================================================================================================================================

/* Spawns the thread to handle this client's connection. (Stores it to be joined later) */
void connected_client::spawn_handle() { 
    
    this->listening_handle = std::thread(&connected_client::t_handle_listening, this); 
    this->sending_handle = std::thread(&connected_client::t_handle_sending, this); 
    
}

// ==============================================================================================================================================================
// Threads ======================================================================================================================================================
// ==============================================================================================================================================================

/* Thread that handles listening for client connection. */
void connected_client::t_handle_listening()  {

    // Runs while the client is not killed.
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
                else if(new_message.compare("/ping") == 0) { // Sends a "pong" back to the client (done here to avoid delays on the queue).       
                    std::string ping_msg = COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " pong";
                    this->send(ping_msg);
                } else // If the request can't be handled here puts it on the request queue.
                    this->server_instance->make_request(this, new_message);
                break;

            case 1: // No new messages from this client. =========================================================================
                break; // If there are no messages nothing is done.

            case -1: // The client has disconnected. =============================================================================
                this->atmc_kill = true; // Kills the connected client. 
                break;

            default: // An error has happened. ===================================================================================
                std::cerr << COLOR_BOLD_RED << "ERROR " << status << "!" << COLOR_DEFAULT << std::endl;
                break;
        }        
    }

}

/* Thread that handles sending messages to the client. */
void connected_client::t_handle_sending() {
    
    // Runs while the client is not killed.
    while(!this->atmc_kill) {

        // Stores if there's currently a message to be sent.
        bool has_message = false;

        // Stores the message being sent.
        std::string current_message;

        // --------------------------------------------------------------------------------------------------------------------------------------------------
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_message_queue.lock();
        // ENTER CRITICAL REGION =======================================
        /* Tries the first message on the queue, modifying the queue can cause problems if the listening handler
        is also adding a message, thus a semaphore is used. */
        if(this->message_queue.size() > 0) {
            current_message = this->message_queue.front(); // Gets the first message on the queue.
            this->message_queue.pop(); // Removes the message from the queue.
            has_message = true; // Marks that there's a message to be sent.
        }
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_message_queue.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // If there's no request, does nothing and looks agains.
        if(!has_message)
            continue; 

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
                    std::cerr << COLOR_BOLD_YELLOW << "Client with socket " << std::to_string(this->client_socket) << " failed to acknowledge message! (" << std::to_string(attempts) << " remaining)" << COLOR_DEFAULT << std::endl;

                // Attempt to send the message.
                send_message(this->client_socket, current_message);
                attempts--;

                // Gets the start time for this attempt.
                start_time = std::chrono::steady_clock::now();
            }

            // Marks if all messages were successfully sent.
            if(this->atmc_ack_received_message < 1)
                success = true;

        }

        // If the client could not confirm the message was received, shut it down.
        if(!success && !this->atmc_kill)
            shutdown(this->client_socket, SHUT_RDWR);

    }

}

// ==============================================================================================================================================================
// Messaging ====================================================================================================================================================
// ==============================================================================================================================================================

/* Adds a new message to queue to be sent to this client. */
void connected_client::send(const std::string &message) {

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_message_queue.lock();
    // ENTER CRITICAL REGION =======================================
    /* Adds the new message to the queue, modifying the queue can cause problems if the send
    handler is also trying to read it at, thus a semaphore is used. */
    this->message_queue.push(message); // Copies the new message to the queue.
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_message_queue.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------
    
}

// ==============================================================================================================================================================
// Getters/setters ==============================================================================================================================================
// ==============================================================================================================================================================

/* Returns this client's nickname (doesn't use a lock, because socket is supposed to never change after being 
assigned by the constructor). */
int connected_client::get_socket() const {
    return this->client_socket;
}

/* Returns this client's nickname. */
std::string connected_client::get_nickname() const {
    return this->nickname;
}

/* Tries updating the player nickname. */
bool connected_client::set_nickname(const std::string &nickname) {

    // Checks if the nickname is valid.
    if(!connected_client::is_valid_nickname(nickname))
        return false;

    // Sets the nickname and returns a success.
    this->nickname = nickname;
    return true;

}

/* Changes the channel this client is connected to. */
void connected_client::set_channel(const std::string &channel_name, int role) {

    this->current_channel = channel_name;
    this->channel_role = role;    

    // Sends a message to the client to enable or disable the admin commands.
    if(role == CLIENT_ROLE_ADMIN) { // Activates showing admin commands.
        std::string admin_on_msg = "/show_admin_commands";
        this->send(admin_on_msg);
    } else {
        std::string admin_off_msg = "/hide_admin_commands";
        this->send(admin_off_msg); // Deactivates showing admin commands.
    }

}

/* Returns the channel this client is connected to. */
std::string connected_client::get_channel() const {
    return this->current_channel;
}

/* Returns the role of this client on it's channel. */
int connected_client::get_role() const {
    return this->channel_role;
}

/* Returns the ip of this client as a string. */
std::string connected_client::get_ip() const {
    
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
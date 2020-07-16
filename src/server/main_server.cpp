// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "main_server.hpp"
# include "channel.hpp"
# include "request.hpp"
# include "connected_client.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <vector>
# include <queue>

# include <thread>
# include <mutex>
# include <atomic>

# include <errno.h>

# include <fcntl.h>
# include <csignal>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// ==============================================================================================================================================================
// Globals ======================================================================================================================================================
// ==============================================================================================================================================================

// Used to indicate when the server should be closed.
std::atomic_bool atmc_close_server_flag(false);

// ==============================================================================================================================================================
// Signals ======================================================================================================================================================
// ==============================================================================================================================================================

// Sets the flag to indicate the server should be closed.
void close_server(int signal_num) { atmc_close_server_flag = true; }

// ==============================================================================================================================================================
// Constructors/destructors =====================================================================================================================================
// ==============================================================================================================================================================

// Creates a new server with a network socket and binds the socket.
server::server(int port_number) { 

    // Creates a TCP socket.
    this->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Sets the socket to be non-blocking.
    int flags = fcntl(this->server_socket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(this->server_socket, F_SETFL, flags);

    // Gets an address for the socket.
    this->server_address.sin_family = AF_INET;
    this->server_address.sin_port = htons(port_number);
    this->server_address.sin_addr.s_addr = INADDR_ANY;

    // Binds the server to the socket.
    this->server_status = bind(this->server_socket, (struct sockaddr *) &(this->server_address), sizeof(this->server_address));

}

// Deletes the server closing sockets and deleting necessary clients and channels.
server::~server() { 

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_new_clients.lock();
    // ENTER CRITICAL REGION =======================================
    // Deletes new client from the new client queue.
    while (!this->new_clients.empty()) {
        // Gets a new client from the queue.
        connected_client *new_client = this->new_clients.front();
        delete new_client; // Deletes the client.
        this->new_clients.pop(); // Removes from the queue.
    } 
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_new_clients.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------

    // Shutdowns and kills any remaining clients.
    for(auto it = this->clients.begin(); it != this->clients.end(); it++) {
        shutdown((*it)->get_socket(), SHUT_RDWR);
        (*it)->atmc_kill = true;
    }

    // Calls check_connections to get rid of the clients.
    this->check_connections();

    // Clears the memory of any remaining channels.
    for(auto it = this->channels.begin(); it != this->channels.end(); it++)
        delete (*it);

    // Closes the socket.
    close(this->server_socket);

}

// ==============================================================================================================================================================
// Server =======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns the status of the server */
int server::get_status() { return this->server_status; }


// Handles the server instance (control of the program is given to the server until it finishes).
void server::handle() {

    // Sets the server to be closed when CTRL+C is pressed.
    std::signal(SIGINT, close_server);

    // Spawns the thread that handles client connections.
    std::thread connections_handler(&server::t_handle_connections, this);

    // Executes until the server is closed, processing client requests.
    while(!atmc_close_server_flag) {

        // Checks for any changes in the client connections before processing a new request.
        server::check_connections();

        // Stores if there's currently a request to be processed.
        bool has_request = false;

        // Stores the request being processed.
        request current_request;

        // --------------------------------------------------------------------------------------------------------------------------------------------------
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_request_queue.lock();
        // ENTER CRITICAL REGION =======================================
        /* Tries the first request on the queue, modifying the queue can cause problems if some client
        handler is also adding a request, thus a semaphore is used. */
        if(this->request_queue.size() > 0) {
            current_request = this->request_queue.front(); // Gets the first request on the queue.
            this->request_queue.pop(); // Removes the request from the queue.
            has_request = true; // Marks that there's a request.
        }
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_request_queue.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // If there's no request, does nothing and looks agains.
        if(!has_request)
            continue;        

        // Gets the client that sent this request.
        connected_client *origin = this->get_client_ref(current_request.get_origin_socket());
        if(origin == nullptr) { // Checks if the client who sent the request is still avaliable.
            std::cerr << "Request cancelled! (Client with socket " << current_request.get_origin_socket() << " is no longer avaliable)";
            continue;
        }

        // Gets the data from the request.
        std::string data =  current_request.get_data();

        // Stores if the request failed because the client doesn't have needed admin rights.
        // Used to send a warning to the client later.
        bool admin_failed = false;

        // ! NOTE: /ack and /ping request are handled immediately and are not put on the request queue to avoid delays.
        // Checks for the type of the request and executes it properly.
        switch (current_request.get_type()) {

            case rt_Send:
                this->send_request(origin, data);
                break;

            case rt_Nickname:
                this->nickname_request(origin, data);
                break;

            case rt_Join:
                this->join_request(origin, data);
                break;

            case rt_Admin_kick:
                if(origin->get_role() == CLIENT_ROLE_ADMIN)
                    this->kick_request(origin, data);
                else admin_failed = true;
                break;

            case rt_Admin_mute:
                if(origin->get_role() == CLIENT_ROLE_ADMIN)
                    this->toggle_mute_request(origin, data, true);
                else admin_failed = true;
                break;

            case rt_Admin_unmute:
                if(origin->get_role() == CLIENT_ROLE_ADMIN)
                    this->toggle_mute_request(origin, data, false);
                else admin_failed = true;
                break;

            case rt_Admin_whois:
                if(origin->get_role() == CLIENT_ROLE_ADMIN)
                    this->whois_request(origin, data);
                else admin_failed = true;
                break;
            
            default:
                break;
        }

        if(admin_failed) // Sends a warning to the client that a request failed because it's not an admin.
            origin->spawn_send_message_worker(new std::string("server: you must be an admin to do that!"));
        
    }

    // Waits for the threads to finish before giving control back to the main program.
    connections_handler.join();

}

// ==============================================================================================================================================================
// Client handling ==============================================================================================================================================
// ==============================================================================================================================================================

/* Separate thread to handle the connection of new clients */
void server::t_handle_connections() {

    // Executes until the server is closed.
    while(!atmc_close_server_flag) {

        // Listens for a new connection.
        listen(this->server_socket, BACKLOG_LEN);

        // Accepts the connection and returns the client socket.
        int new_client_socket = accept(this->server_socket, nullptr, nullptr);

        // If no connection happened, checks why.
        if(new_client_socket == -1) {

            // No connection is avaliable, nothing to do here.
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            // Other types of errors.
            std::cerr << "Unidentified connection error!" << std::endl;
            continue;
            
        }

        // Creates a new connection object and assigns the socket.
        connected_client *new_connection = new connected_client(new_client_socket, this);

        if(new_connection == nullptr) { // Checks for errors creating the connection.
            std::cerr << "Error creating new connection!" << std::endl;
            continue;
        }

        // --------------------------------------------------------------------------------------------------------------------------------------------------
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_new_clients.lock();
        // ENTER CRITICAL REGION =======================================
        /* Adds the new connection to the queue, modifying the queue can cause problems if some 
        client handler is reading it at the same time, thus a semaphore is used. */
        this->new_clients.push(new_connection);
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_new_clients.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        std::cerr << "Client just connected with socket " << new_connection->get_socket() << "!" << std::endl;

    }

}

/* Checks for changes in client conenctions. Adding or removing them if necessary. */
void server::check_connections() {
    
    // Checks for disconenctions and removes them.
    auto iter = this->clients.begin();
    while (iter != this->clients.end()) {
        if((*iter)->atmc_kill) { // Checks if the client has disconnected and needs to be killed.
             std::cerr << "Client with socket " << (*iter)->get_socket() << " disconnected!" << std::endl;
            // Kills the client.
            kill_client(*iter);
            // Removes the client from the list.
            this->clients.erase(iter);
            // Restarts the check.
            iter = this->clients.begin();
            continue;
        }
        iter++;   
    }

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_new_clients.lock();
    // ENTER CRITICAL REGION =======================================

    // Transfers any new clients to the main list.
    while (!this->new_clients.empty()) {
        // Gets a new client from the queue.
        connected_client *new_client = this->new_clients.front();
        // Spawns the thread to handle the client connection.
        new_client->spawn_handle();
        this->clients.insert(new_client); // Transfer the client.
        this->new_clients.pop(); // Removes from the queue.
    }    

    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_new_clients.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------
}

// Kills a client that has disconnected from the server, performing any cleanup necessary.
void server::kill_client(connected_client *connection) {

    // TODO: channel cleanup.

    // Deletes the client connection.
    delete connection;

}

// ==============================================================================================================================================================
// Creates/deletes channels =====================================================================================================================================
// ==============================================================================================================================================================

bool server::create_channel(std::string name, int admin_socket) {

    // Checks if the channel name is valid.
    if(!channel::is_valid_channel_name(name))
        return false;

    // Creates the channel.
    channel *new_channel = new channel(this->channels.size(), name, this);

    // Adds the admin to the channel.
    new_channel->add_member(admin_socket);

    // Creates the new channel and adds it to the list.
    this->channels.push_back(new_channel);

    std::cerr << "Channel " << name << " created!" << std::endl;

    return true;

}

// !FIXME: Segmentation fault.
// TODO: Improve thread safety, currently this function is locked all the times it's called so locking again will cause a deadlock, but it would be better to lock inside the function.
// Deletes a new channel on this server.
bool server::delete_channel(int index) {

    // TODO: currently there's no need to remove users from the channel, as the channel will only be removed when empty, but this should be done here.
    // Removes the channel.
    std::string name = this->channels[index]->get_name(); // Gets the name to the log message.
    this->channels.erase(this->channels.begin() + index);

    std::cerr << "Channel " << name << " deleted!" << std::endl;

    return true;

}

// ==============================================================================================================================================================
// Getters ======================================================================================================================================================
// ==============================================================================================================================================================

/* Returns a reference to a client with a certain socket. */
connected_client *server::get_client_ref(int socket) {

    // Searches for the client in the list.
    for(auto it = this->clients.begin(); it != this->clients.end(); it++)
        if((*it)->get_socket() == socket)
            return (*it);

    return nullptr;

}

/* Returns a reference to a client with a certain nickname. */
connected_client *server::get_client_ref(std::string &nickname) {

    // Searches for the client in the list.
    for(auto it = this->clients.begin(); it != this->clients.end(); it++)
        if((*it)->get_nickname().compare(nickname) == 0)
            return (*it);

    return nullptr;

}

/* Returns a reference to a channel with a certain index. */
channel *server::get_channel_ref(int index) {

    // Gets the channel at a valid index or keeps a nullptr.
    if((size_t)index > 0 && (size_t)index < this->channels.size())
        return this->channels[index];

    return nullptr;

}

/* Returns a reference to a channel with a certain name. */
channel *server::get_channel_ref(std::string &name) {

    // Searches for the channel in the list.
    for(auto it = this->channels.begin(); it != this->channels.end(); it++)
        if((*it)->get_name().compare(name) == 0)
            return (*it);

    return nullptr;

}

// ==============================================================================================================================================================
// Requests =====================================================================================================================================================
// ==============================================================================================================================================================

/* Makes a request to the server, that will be added to the request queue and handled as soon as possible (gets a lock to the request_queue during execution) */
void server::make_request(connected_client *origin, std::string content) {

    // Gets the origin socket to be used in execution.
    int origin_socket = origin->get_socket();

    // Checks for a valid request, any empty request or one without a "/" as the first character can be discarded.
    if(!content.empty() && content[0] == '/') {

        // Gets the position that divides the request command from it's data.
        size_t delimiter = content.find(' ');

        // Breaks the request into command and data parts.
        std::string command = content.substr(0,delimiter);
        std::string data = std::string(); // Initializes as empty string.
        try { // Tries getting the data portion. (try-catch is needed because sometimes the data portion may not exist)
            data = content.substr(delimiter+1,content.size());
        } catch (const std::out_of_range& oor) {
            // Does nothing, simple leaves data as an empty string.
        }

        // ! NOTE: /ack and /ping request are handled immediately and are not put on the request queue to avoid delays.
        // Detects the type of the request.
        request_type r_type = rt_Invalid;
        if(!data.empty()) {
            if (command.compare("/send") == 0)
                r_type = rt_Send;
            else if(command.compare("/nickname") == 0)
                r_type = rt_Send;           
            else if(command.compare("/join") == 0)
                r_type = rt_Join;
            else if(command.compare("/kick") == 0)
                r_type = rt_Admin_kick;
            else if(command.compare("/mute") == 0)
                r_type = rt_Admin_mute;
            else if(command.compare("/unmute") == 0)
                r_type = rt_Admin_unmute;
            else if(command.compare("/whois") == 0)
                r_type = rt_Admin_whois;
        }

        // If the request type is invalid sends a warning back to the client and ignores it.
        if(r_type == rt_Invalid) {
            origin->spawn_send_message_worker(new std::string("server: invalid command or command parameters!"));
            return;
        }

        // Everything is correct, creates the request.
        request new_request(origin_socket, r_type, data);

        // --------------------------------------------------------------------------------------------------------------------------------------------------
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_request_queue.lock();
        // ENTER CRITICAL REGION =======================================
        /* Adds the new request to the queue, modifying the queue can cause problems if some client
        handler is also adding a request or if the server is reading a request to be executed at
        the same time, thus a semaphore is used. */
        this->request_queue.push(new_request); // Adds the new request to the queue.
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_request_queue.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        if(content.size() > 20)
            std::cerr << "New request from socket " << origin_socket << ": \"" << content.substr(0, 20) << "...\"" << std::endl;
        else
            std::cerr << "New request from socket " << origin_socket << ": \"" << content << "\"" << std::endl;

    }

    std::cerr << "Invalid request from socket " << origin_socket + "! Ignoring...";

}

/* Sends a message from a client to other clients on it's channel. */
void server::send_request(connected_client *origin, std::string &message) {

    // Gets the client's channel.
    int target_channel_index = origin->get_channel();

    // If the client is not on a valid channel sends an error message.
    if(target_channel_index < 0) {
        origin->spawn_send_message_worker(new std::string("server: you need to join a channel before sending messages!"));
        return;
    }

    // Gets a reference to the client's channel.
    channel *target_channel = this->get_channel_ref(target_channel_index);

    // Checks if the client is not muted.
    if(!target_channel->is_muted(origin->get_socket())) {

        // Gets the client's nickname.
        std::string client_name = origin->get_nickname();

        // Gets the channel's name.
        std::string channel_name = target_channel->get_name();

        // Gets the target sockets (channel's members).
        int num_targets;
        int *message_targets = target_channel->get_members(&num_targets);

        // Sends the message to each target.
        for(int i = 0; i < num_targets; i++) {
            // Gets the target client.
            connected_client *target_client = this->get_client_ref(message_targets[i]);
            if(target_client != nullptr) {
                target_client->spawn_send_message_worker(new std::string(channel_name + " " + client_name + " " + message));
            }
        }

        // Deletes the targets array.
        delete[] message_targets;

    } else { // Sends a message warning the client that it is muted.
        origin->spawn_send_message_worker(new std::string("server: you are currently muted on the channel!"));
        return;
    }

}

/* Tries changing the nickname of a certain client. */
void server::nickname_request(connected_client *origin, std::string &nickname) {

    // Checks if the nickname doesn't exist on the server.
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    if(this->get_client_ref(nickname) != nullptr) {
        origin->spawn_send_message_worker(new std::string("server: this nickname already exists!"));
        return;
    }

    // Tries updating the nickname and sends a message to the client telling the results.
    if(origin->set_nickname(nickname))
        origin->spawn_send_message_worker(new std::string("server: your nickname was changed to " + nickname + "!"));
    else
        origin->spawn_send_message_worker(new std::string("server: this nickname is invalid! (It can't start with '#' or '&' and must not contain spaces or commas)"));

}

/* Tries joining a channel with a certain name as a certain client, tries creating the channel if it doesn't exist. */
void server::join_request(connected_client *origin, std::string &channel_name) {
    // TODO: join request.
    origin->spawn_send_message_worker(new std::string("/join request not available..."));
}

/* Tries kicking a client that must be in the same channel. */
void server::kick_request(connected_client *origin, std::string &nickname) {
    // TODO: kick request only for admins.
    origin->spawn_send_message_worker(new std::string("/kick request not available..."));
}

/* Tries mutting/unmutting a client that must be in the same channel and must not already be muted/unmuted. */
void server::toggle_mute_request(connected_client *origin, std::string &nickname, bool muted) {
    // TODO: mute/unmute request only for admins.
    origin->spawn_send_message_worker(new std::string("/mute/unmute request not available..."));
}

/* Tries finding and showing the IP of a cçient a player that must be in the same channel. */
void server::whois_request(connected_client *origin, std::string &nickname) {
    // TODO: whois request only for admins.
    origin->spawn_send_message_worker(new std::string("/whois request not available..."));
}

// ==============================================================================================================================================================
// Commands =====================================================================================================================================================
// ==============================================================================================================================================================
/*

// Tries joining a server channel.
bool connected_client::join_channel(std::string channel_name) {

    // Searches for the target channel.
    channel *target_channel = nullptr;
    for(auto it = this->server_instance->channels.begin(); it != this->server_instance->channels.end(); it++) {
        if((*it)->name.compare(channel_name) == 0) {
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
            std::thread worker_enable_commands(&connected_client::t_send_message_worker, this, new std::string("/show_admin_commands"));
            worker_enable_commands.detach();

            // Sends a message with text to warn the client of the success.
            std::thread worker_warning(&connected_client::t_send_message_worker, this, new std::string("server: you're now on channel " + channel_name + " as an admin!"));
            worker_warning.detach();

        } else {
            std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: the channel " + channel_name + " doesn't exist and can't be created! (Invalid name: it has to start with either '#' or '&' and must not contain spaces or commas)"));
            worker.detach();
        }

    } else { // If a channel exists tries joining it.

        // Tries changing the channel.
        success = target_channel->add_client(this);

        // Sends a message to disable showing the admin commands.
        std::thread worker_enable_commands(&connected_client::t_send_message_worker, this, new std::string("/hide_admin_commands"));
        worker_enable_commands.detach();

        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: you're now on channel " + channel_name + "!"));
        worker.detach();

    }

    return success;

}

// Kicks the client with the given nickname.
bool connected_client::kick_client(std::string client_name) {

    // Gets the target client.
    connected_client *target_client = this->server_instance->l_get_client_by_name(client_name);

    // If the client wasn't found returns.
    if(target_client == nullptr) {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: no client with nickname " + client_name + "!"));
        worker.detach();
        return false;
    }

    // Checks if the this client and the target are on the same channel.
    if(target_client->l_get_channel() == this->l_get_channel()) {

        // Removes the client.
        shutdown(target_client->client_socket, SHUT_RDWR);
        this->server_instance->remove_client(target_client);

        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: kicked " + client_name + "!"));
        worker.detach();

        return true;

    } else {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: the client needs to be in the same channel as you for this action!"));
        worker.detach();
        return false;
    }

}

// Mutes the client with the given nickname on the curren channel.
bool connected_client::toggle_mute_client(std::string client_name, bool muted) {

    // Gets the target client.
    connected_client *target_client = this->server_instance->l_get_client_by_name(client_name);

    // If the client wasn't found returns.
    if(target_client == nullptr) {
        // Sends a message with the results.
        std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: no client with nickname " + client_name + "!"));
        admin_worker.detach();
        return false;
    }

    // Checks if the this client and the target are on the same channel.
    int target_channel = this->l_get_channel();
    if(target_channel == target_client->l_get_channel()) {

        // Checks if the nickname doesn't exist on the server.
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->server_instance->updating_channels.lock();
        // ENTER CRITICAL REGION =======================================   
       // Mutes or unmutes the client on the channel.
        bool success = this->server_instance->channels[target_channel]->l_toggle_mute_client(target_client, muted);
        std::string target_channel_name = this->server_instance->channels[target_channel]->name;
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->server_instance->updating_channels.unlock();

        // Sends a message with the results.
        if(muted) {

            if(success) {

                // Creates the worker thread for the admin.
                std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: muted " + client_name + "!"));
                admin_worker.detach();

                // Creates the worker thread for the target.
                std::thread target_worker(&connected_client::t_send_message_worker, target_client, new std::string("server: you have been muted on channel " + target_channel_name + "!"));
                target_worker.detach();
            } else {

                // Creates the worker thread for the admin.
                std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: " + client_name + " is already muted!"));
                admin_worker.detach();

            }

        } else {

            if(success) {

                // Creates the worker thread for the admin.
                std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: un-muted " + client_name + "!"));
                admin_worker.detach();

                // Creates the worker thread for the target.
                std::thread target_worker(&connected_client::t_send_message_worker, target_client, new std::string("server: you have been un-muted on channel " + target_channel_name + "!"));
                target_worker.detach();

            } else {

                // Creates the worker thread for the admin.
                std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: " + client_name + " isn't muted!"));
                admin_worker.detach();

            }
        }

        return success;

    } else {
        // Sends a message with the results.
        std::thread admin_worker(&connected_client::t_send_message_worker, this, new std::string("server: the client needs to be in the same channel as you for this action!"));
        admin_worker.detach();
        return false;
    }

}

// Prints the IP of a client to the admin.
bool connected_client::whois_client(std::string client_name) {

    // Gets the target client.
    connected_client *target_client = this->server_instance->l_get_client_by_name(client_name);

    // If the client wasn't found returns.
    if(target_client == nullptr) {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: no client with nickname " + client_name + "!"));
        worker.detach();
        return false;
    }

    // Checks if the this client and the target are on the same channel.
    if(target_client->l_get_channel() == this->l_get_channel()) {

        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: the ip for " + client_name + " is " + target_client->l_get_ip() + "."));
        worker.detach();

        return true;

    } else {
        // Sends a message with the results.
        std::thread worker(&connected_client::t_send_message_worker, this, new std::string("server: the client needs to be in the same channel as you for this action!"));
        worker.detach();
        return false;
    }   
}
*/
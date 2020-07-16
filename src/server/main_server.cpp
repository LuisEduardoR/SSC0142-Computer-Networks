// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "../color.hpp"

# include "main_server.hpp"
# include "channel.hpp"
# include "request.hpp"
# include "connected_client.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <map>
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

    // Calls check_channels to get rid of the channels.
    this->check_channels();

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

        // Checks for any empty channels that should be removed before processing a new request.
        server::check_channels();

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
            std::cerr << COLOR_YELLOW << "Request cancelled! (Client with socket " << current_request.get_origin_socket() << COLOR_YELLOW << " is no longer avaliable)";
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

        if(admin_failed) { // Sends a warning to the client that a request failed because it's not an admin.
            std::string error_msg = COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " you must be an admin to do that!";
            origin->send(error_msg);
        }
        
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
            std::cerr << COLOR_RED << "Unidentified connection error!" << COLOR_DEFAULT << std::endl;
            continue;
            
        }

        // Creates a new connection object and assigns the socket.
        connected_client *new_connection = new connected_client(new_client_socket, this);

        if(new_connection == nullptr) { // Checks for errors creating the connection.
            std::cerr << COLOR_RED << "Error creating new connection!" << COLOR_DEFAULT << std::endl;
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

        std::cerr << COLOR_BLUE << "Client just connected with socket " << new_connection->get_socket() << "!" << COLOR_DEFAULT << std::endl;

    }

}

/* Checks for changes in client conenctions. Adding or removing them if necessary. */
void server::check_connections() {
    
    // Checks for disconenctions and removes them.
    auto iter = this->clients.begin();
    while (iter != this->clients.end()) {
        if((*iter)->atmc_kill) { // Checks if the client has disconnected and needs to be killed.
             std::cerr << COLOR_YELLOW << "Client with socket " << (*iter)->get_socket() << " disconnected!" << COLOR_DEFAULT << std::endl;
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

/* Checks for channels that became empty and can be deleted. */
void server::check_channels() {

    // Gets each empty channel.
    while (!this->empty_channels.empty()) {

        // Gets the start of the queue.
        std::string target_name = this->empty_channels.front();
        this->empty_channels.pop(); // Removes the name from the queue.

        // Deletes the channel.
        this->delete_channel(target_name);

    } 

}

// Kills a client that has disconnected from the server, performing any cleanup necessary.
void server::kill_client(connected_client *connection) {

    // Gets the client's channel.
    std::string channel_name = connection->get_channel();
    channel *target_channel = this->get_channel_ref(channel_name);

    // If the client is currently on a channel removes him from that channel.
    if(target_channel != nullptr) {

        // Removes the client from current channel.
        target_channel->remove_member(connection->get_socket());
        std::string no_channel("NONE");

        // Sets the client to being in no channel.
        connection->set_channel(no_channel, CLIENT_NO_CHANNEL);

        // Adds the channel to the empty list if it became empty.
        if(target_channel->is_empty()) {
            std::cerr << "Channel " << target_channel->get_name() << " is empty and will soon be deleted!" << std::endl;
            this->empty_channels.push(target_channel->get_name());
        }
    }

    // Deletes the client connection.
    delete connection;

}

// ==============================================================================================================================================================
// Creates/deletes channels =====================================================================================================================================
// ==============================================================================================================================================================

bool server::create_channel(std::string &channel_name, int admin_socket) {

    // Checks if the channel name is valid.
    if(!channel::is_valid_channel_name(channel_name))
        return false;

    // Creates the channel.
    channel *new_channel = new channel(channel_name, this);

    // Adds the admin to the channel.
    new_channel->add_member(admin_socket);

    // Creates the new channel and adds it to the map.
    this->channels.insert(std::make_pair(channel_name, new_channel));

    std::cerr << COLOR_BLUE << "Channel " << channel_name << " created!" << COLOR_DEFAULT << std::endl;

    return true;

}

/* Deletes an empty channel on this server. */
bool server::delete_channel(std::string &channel_name) {

    // Gets a reference to the channel.
    channel *target = this->get_channel_ref(channel_name);

    // Checks if the channel being deleted exists.
    if(target == nullptr) {
        std::cerr << COLOR_BOLD_RED << "Channel " + channel_name + " doesn't exist!" << COLOR_DEFAULT << std::endl;
        return false;
    }

    // Gives an error if the channel is not empty.
    if(!target->is_empty()) {
        std::cerr << COLOR_BOLD_RED << "Channel " + channel_name + " is not empty!" << COLOR_DEFAULT << std::endl;
        return false;
    }

    // Removes the channel from the map.
    this->channels.erase(channel_name);

    // Deletes the channel freeing memory.
    delete target;

    std::cerr << COLOR_YELLOW << "Channel " << channel_name << " deleted!" << COLOR_DEFAULT << std::endl;

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

/* Returns a reference to a channel with a certain name. */
channel *server::get_channel_ref(std::string &channel_name) {

    // Searches for the channel in the list.
    auto it = channels.find(channel_name);
    if(it != this->channels.end())
        return (*it).second;

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
                r_type = rt_Nickname;           
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
            std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " invalid command or command parameters!");
            origin->send(error_msg);
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

        return;

    }

    if(content.size() > 20)
        std::cerr << "Invalid request from socket " << origin_socket << ": \"" << content.substr(0, 20) << "...\"! Ignoring..." << std::endl;
    else
        std::cerr << "Invalid request from socket " << origin_socket << ": \"" << content << "\"! Ignoring..." << std::endl;

}

/* Sends a message from a client to other clients on it's channel. */
void server::send_request(connected_client *origin, std::string &message) {

    // Gets the client's channel.
    std::string target_channel_name = origin->get_channel();
    channel *target_channel = this->get_channel_ref(target_channel_name);

    // If the client is not on a valid channel sends an error message.
    if(target_channel == nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " you need to join a channel before sending messages!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Checks if the client is not muted.
    if(!target_channel->is_muted(origin->get_socket())) {

        // Gets the client's nickname.
        std::string client_name = origin->get_nickname();

        // Gets the target sockets (channel's members).
        int num_targets;
        int *message_targets = target_channel->get_members(&num_targets);

        // Sends the message to each target.
        for(int i = 0; i < num_targets; i++) {
            // Gets the target client.
            connected_client *target_client = this->get_client_ref(message_targets[i]);
            if(target_client != nullptr) {
                std::string complete_message = COLOR_BLUE + target_channel_name + COLOR_CYAN + " " + client_name + ": " + COLOR_DEFAULT + message;
                target_client->send(complete_message);
            }
        }

        // Deletes the targets array.
        delete[] message_targets;

    } else { // Sends a message warning the client that it is muted.
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_YELLOW + " you are currently muted on the channel " + target_channel_name + "!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

}

/* Tries changing the nickname of a certain client. */
void server::nickname_request(connected_client *origin, std::string &nickname) {

    // Checks if the nickname doesn't exist on the server.
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    if(this->get_client_ref(nickname) != nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " this nickname already exists!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Tries updating the nickname and sends a message to the client telling the results.
    if(origin->set_nickname(nickname)) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " your nickname was changed to " + nickname + "!");
        origin->send(error_msg);
    } else {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " this nickname is invalid! (It can't start with '#' or '&' and must not contain spaces or commas)" + COLOR_DEFAULT);
        origin->send(error_msg);
    }

}

/* Tries joining a channel with a certain name as a certain client, tries creating the channel if it doesn't exist. */
void server::join_request(connected_client *origin, std::string &channel_name) {

    // Checks for an invalid channel name, and sends a warning to the client.
    if(!channel::is_valid_channel_name(channel_name)) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " this channel name is invalid! (It can't start with '#' or '&' and must not contain spaces or commas)" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Gets the client's channel.
    std::string target_channel_name = origin->get_channel();
    channel *target_channel = this->get_channel_ref(target_channel_name);

    // If the client is currently on a channel removes him from that channel.
    if(target_channel != nullptr) {

        // Removes the client from current channel.
        target_channel->remove_member(origin->get_socket());

        // Sets the client to being in no channel.
        std::string no_channel("NONE");
        origin->set_channel(no_channel, CLIENT_NO_CHANNEL);

        // Adds the channel to the empty list if it became empty.
        if(target_channel->is_empty()) {
            std::cerr << "Channel " << target_channel->get_name() << " is empty and will soon be deleted!" << std::endl;
            this->empty_channels.push(target_channel->get_name());
        }

    }

    // Gets a reference to the channel if it already exists.
    auto iter = this->channels.find(channel_name);
    target_channel = nullptr;
    if(iter != this->channels.end())
        target_channel = iter->second;

    // If the reference could be obtained the channel already exists, so adds the client.
    if(target_channel != nullptr) {
        target_channel->add_member(origin->get_socket()); // Adds the client.
        origin->set_channel(channel_name, CLIENT_ROLE_NORMAL); // Sets the client channel and role.
        std::string join_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " you're now on channel " + channel_name + "!");
        origin->send(join_msg);
        return;
    }

    // If no reference was found them creates the new channel with the client as an admin.
    create_channel(channel_name, origin->get_socket()); // Creates the channel.
    origin->set_channel(channel_name, CLIENT_ROLE_ADMIN); // Sets the client channel and role.
    std::string join_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " you're now on channel " + channel_name + " as an " + COLOR_BOLD_BLUE + "admin" + COLOR_DEFAULT + "!");
    origin->send(join_msg);

}

/* Tries kicking a client that must be in the same channel. */
void server::kick_request(connected_client *origin, std::string &nickname) {

    // Gets a reference to the target client that will be kicked.
    connected_client *target = this->get_client_ref(nickname);

    // ? Shoudn't we only be able to kick clients in the same channel we are the admin???

    // Checks if the target client exists and sends an error message if it does not.
    if(target == nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " could not find client with nickname \"" + nickname + "\"!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Shutdowns the target client connection.
    shutdown(target->get_socket(), SHUT_RDWR);

    // Sends a message telling the admin that the client was kicked.
    std::string kick_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " \"" + nickname + "\" kicked!");
    origin->send(kick_msg);

}

/* Tries mutting/unmutting a client that must be in the same channel and must not already be muted/unmuted. */
void server::toggle_mute_request(connected_client *origin, std::string &nickname, bool muted) {

    // Gets a reference to the target client that will have it's ip sent.
    connected_client *target_client = this->get_client_ref(nickname);

    // Checks if the target client exists and sends an error message if it does not.
    if(target_client == nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " could not find client with nickname \"" + nickname + "\"!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Gets a reference to the target channel in which the admin is.
    std::string target_channel_name = origin->get_channel();
    channel *target_channel = this->get_channel_ref(target_channel_name);

    // If the admin is not on a valid channel sends an error message.
    if(target_channel == nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " you need to join a channel before doing this!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Ensures admin and client are in the same channel, sends an error message if they are not.
    if(origin->get_channel().compare(target_client->get_channel()) != 0) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " you must be in the same channel as \"" + nickname + "\" to do that!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }   

    // Tries muting the target client.
    bool success = target_channel->toggle_mute_member(target_client->get_socket(), muted);

    // Sends a message with the results.
    if(muted) {

        // Sends success message.
        if(success) {
            
            // Sends message to the admin.            
            std::string mute_msg = COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " \"" + nickname + "\" is now muted!";
            origin->send(mute_msg);

            // Sends message to the target.
            mute_msg = COLOR_MAGENTA + "server:" + COLOR_YELLOW + " you are now muted on the current channel!" + COLOR_DEFAULT;
            target_client->send(mute_msg);

        } else { // Sends error message.
            std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " \"" + nickname + "\" is not currently muted!" + COLOR_DEFAULT);
            origin->send(error_msg);
        }

    } else {

        // Sends success message.
        if(success) {

            // Sends message to the admin.            
            std::string unmute_msg = COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " \"" + nickname + "\" is no longer muted!";
            origin->send(unmute_msg);

            // Sends message to the target.
            unmute_msg = COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " you are no longer muted on the current channel!";
            target_client->send(unmute_msg);
            
        } else { // Sends error message.
            std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " \"" + nickname + "\" is not currently muted!" + COLOR_DEFAULT);
            origin->send(error_msg);
        }

    }

}

/* Tries finding and showing the IP of a client a player that must be in the same channel. */
void server::whois_request(connected_client *origin, std::string &nickname) {
    
    // Gets a reference to the target client that will have it's ip sent.
    connected_client *target = this->get_client_ref(nickname);

    // Checks if the target client exists and sends an error message if it does not.
    if(target == nullptr) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " could not find client with nickname \"" + nickname + "\"!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Ensures admin and client are in the same channel, sends an error message if they are not.
    if(origin->get_channel().compare(target->get_channel()) != 0) {
        std::string error_msg(COLOR_MAGENTA + "server:" + COLOR_RED + " you must be in the same channel as \"" + nickname + "\" to do that!" + COLOR_DEFAULT);
        origin->send(error_msg);
        return;
    }

    // Sends a message telling the admin the IP of the target.
    std::string kick_msg(COLOR_MAGENTA + "server:" + COLOR_DEFAULT + " the IP address of \"" + nickname + "\" is " + target->get_ip() + "!");
    origin->send(kick_msg);

}
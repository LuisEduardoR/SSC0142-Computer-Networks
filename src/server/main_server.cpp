// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "main_server.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <errno.h>

# include <fcntl.h>
# include <csignal>

# include <mutex>
# include <thread>
# include <atomic>

# include <sys/types.h>
# include <sys/socket.h>

# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the server should be closed.
std::atomic_bool atmc_close_server_flag(false);

// Sets the flag to indicate the server should be closed.
void close_server(int signal_num) { atmc_close_server_flag = true; }

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

    // Initializes the client connections list.
    client_connections.clear();

}

// DESTRUCTOR
server::~server() { 

    // Closes the socket.
    close(this->server_socket);

    // Kills all connected clients.
    for(auto it = this->client_connections.begin(); it != this->client_connections.end(); it++)
        (*it)->atmc_kill = true; // Marks that this clients is to be killed.

    // Clears the memory of any remaining channels.
    for(auto it = this->channels.begin(); it != this->channels.end(); it++)
        delete (*it);

}

// Handles the server instance (control of the program is given to the server until it finishes).
void server::handle() {

    // Sets the server to be closed when CTRL+C is pressed.
    std::signal(SIGINT, close_server);

    // Spawns the thread that handles client connections.
    std::thread t_handle_connections(&server::thread_handle_connections, this);

    // Spawns the thread that handles conencted client requests.
    std::thread t_handle_requests(&server::thread_handle_requests, this);

    // Waits for the threads to finish before giving control back to the main program.
    t_handle_connections.join();
    t_handle_requests.join();

}

/* Returns the status of the server */
int server::get_status() { return this->server_status; }

/* Handles the connection of new clients */
void server::thread_handle_connections() {

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

        // Spawns the thread to handle the client connection.
        new_connection->spawn_handle();

        // --------------------------------------------------------------------------------------------------------------------------------------------------
        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_clients.lock();
        // ENTER CRITICAL REGION =======================================
        /* Adds the new connection to the list, modifying the list can cause problems if some 
        client handler is reading it at the same time, thus a semaphore is used. */
        this->clients.insert(new_connection);
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_clients.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        std::cerr << "Client just connected with socket " << new_connection->client_socket << "!" << std::endl;

    }

}

/* Handles the execution of client requests. */
void server::thread_handle_requests() {

     // Executes until the server is closed.
    while(!atmc_close_server_flag) {

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
        
        
        std::string content = current_request.get_content();
        // NOTE: /ack and /ping request are handled immediately and are not put on the request queue.
        // Detects a request to send a message on the client's current channel.
        if (content.substr(0,6).compare("/send ") == 0) {

            // TODO: send request.
            // Sends a message to the channel.
            //this->l_send_to_channel(received_message.substr(6,received_message.length()));

        // Detects the nickname command, tries changing the client nickname and sends a message telling the results.
        } else if(content.substr(0,10).compare("/nickname ") == 0) { 

            // Looks for the nickname part of the request.
            std::string nickname = content.substr(10,content.length());

            // Gets the client that sent this request.
            connected_client *target = this->l_get_client_ref(current_request.get_origin_socket());

            if(target == nullptr) { // !FIXME: always entering here V.
                std::cerr << "/nickname request cancelled! (Client with socket " << current_request.get_origin_socket() << " is no longer avaliable)";
                continue;
            }

            // Executes the request.
            this->nickname_request(target, nickname);

            

        // Detects the join command, tries joining a channel and sends a message telling the results.
        } else if(content.compare("/join ") == 0) { 

            // TODO: join request.
            // Try joining the channel with the name provided.
            //this->join_channel(received_message.substr(6,received_message.length()));
        
        // Checks for the admin commands, if the client is an admin.
        } else if(content.substr(0,6).compare("/kick ") == 0) { 
                
            // TODO: kick request only for admins.
            // Try kicking the client with a certain nickname.
            //this->kick_client(received_message.substr(6,received_message.length()));

        } else if(content.substr(0,6).compare("/mute ") == 0) { 
            
            // TODO: mute request only for admins.
            // Try muting the client with a certain nickname.
            //this->toggle_mute_client(received_message.substr(6,received_message.length()), true);

        } else if(content.substr(0,8).compare("/unmute ") == 0) { 
            
            // TODO: unmute request only for admins.
            // Try unmuting the client with a certain nickname.
            //this->toggle_mute_client(received_message.substr(8,received_message.length()), false);

        } else if(content.substr(0,7).compare("/whois ") == 0) { 

            // TODO: whois request only for admins.
            // Try printing the ip of a certain client.
            //this->whois_client(received_message.substr(7,received_message.length()));

        }       

        // TODO: remove this V.
        std::cerr << "Would execute request from socket " << current_request.get_origin_socket() << ": \"" << current_request.get_content() << "\"" << std::endl;

    }

}

// Used as a thread to check for connecting clients.
void server::t_check_for_connections() {

    while(!atmc_close_server_flag)
    {

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
        this->updating_clients.lock();
        // ENTER CRITICAL REGION =======================================
        /* Adding a client to the list can cause problems if some thread is trying to search for a 
        client, thus a semaphore is used. */
        // Adds the new connection to the list.
        this->client_connections.push_back(new_connection);
        // EXIT CRITICAL REGION ========================================
        // Exits the critical region, and opens the semaphore.
        this->updating_clients.unlock();
        // --------------------------------------------------------------------------------------------------------------------------------------------------

        // Creates a new thread to handle the connection.
        new_connection->spawn_handle();

        std::cerr << "Client just connected with socket " << new_connection->client_socket << "!" << std::endl;

    }

}

// Removes a client that has disconnected from the server.
void server::remove_client(connected_client *connection) {

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_clients.lock();
    // ENTER CRITICAL REGION =======================================
    /* removing a client from the list can cause problems if some thread is trying to search for a 
    client, thus a semaphore is used. */
    // Removes the connection from the list.
    this->clients.erase(this->clients.find(connection));
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_clients.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------

    std::cerr << "Client with socket " << connection->client_socket << " disconnected!" << std::endl;

}

bool server::create_channel(std::string name, connected_client *admin) {

    // Checks if the channel name is valid.
    if(name.length() > MAX_CHANNEL_NAME_SIZE) // Checks for valid size.
        return false;
    // Checks for a valid channel name.
    // It has to start with one of the following characters.
    if(name[0] != '&' && name[0] != '#')
        return false;
    //It can't contain any of the following characters.
    for(auto it = name.begin(); it != name.end(); it++) { 
        if(*it == ' ' || *it == 7 || *it == ',')
            return false;
    }

    // Removes the admin from his current channel if his already in one.
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_channels.lock();

    // ENTER CRITICAL REGION =======================================
    int channel_index = admin->l_get_channel();

    // Removes the client from the old channel if there is one.
    if(channel_index >= 0)
        this->channels[channel_index]->remove_client(admin);

    // Creates the channel.
    channel *new_channel = new channel(this->channels.size(), name, this);

    // Adds the admin to the channel.
    new_channel->members.insert(admin);
    admin->l_set_channel(new_channel->index, CLIENT_ROLE_ADMIN);

    // Creates the new channel and adds it to the list.
    this->channels.push_back(new_channel);

    std::cerr << "Channel " << name << " created!" << std::endl;
    std::cerr << "Client with socket " << admin->client_socket << " is now on channel " << new_channel->name << " as admin! ";
    std::cerr << "(Channel members: " << std::to_string(new_channel->members.size()) << ")" << std::endl;
    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_channels.unlock();

    return true;

}

// !FIXME: Segmentation fault.
// TODO: Improve thread safety, currently this function is locked all the times it's called so locking again will cause a deadlock, but it would be better to lock inside the function.
// Deletes a new channel on this server.
bool server::delete_channel(int index) {

    // TODO: currently there's no need to remove users from the channel, as the channel will only be removed when empty, but this should be done here.
    // Removes the channel.
    std::string name = this->channels[index]->name; // Gets the name to the log message.
    this->channels.erase(this->channels.begin() + index);

    std::cerr << "Channel " << name << " deleted!" << std::endl;

    return true;

}

// Gets a reference to a client with a certain nickname.
connected_client *server::l_get_client_by_name(std::string client_nickname) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_connections.lock();

    // ENTER CRITICAL REGION =======================================
    // Searches for the client with the given nickname on the connected client list.
    connected_client *client = nullptr;
    for(auto it = this->client_connections.begin(); it != this->client_connections.end(); it++) {
        if((*it)->nickname.compare(client_nickname) == 0) {
            client = (*it);
            break;
        }
    }
    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_connections.unlock();

    return client;

}

/* Makes a request to the server, that will be added to the request queue and handled as soon as possible (gets a lock to the request_queue during execution) */
void server::make_request(request new_request) {

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

    std::cerr << "New request from socket " << new_request.get_origin_socket() << ": \"" << new_request.get_content() << "\"" << std::endl;

}

/* Returns a reference to a client with a certain socket. (gets a lock to the clients list during execution) */
connected_client *server::l_get_client_ref(int socket) {

    connected_client *target = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_clients.lock();
    // ENTER CRITICAL REGION =======================================
    /* Reading the list can cause problems if some thread is trying to add or remove a client,
    thus a semaphore is used. */
    // Searches for the client in the list.
    for(auto it = this->clients.begin(); it != this->clients.end(); it++) {
        if((*it)->client_socket == socket) {
            target = (*it);
            break;
        }
    }
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_clients.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------

    // Returns a pointer to the client or nullptr if no client was found.
    return target;

}

/* Returns a reference to a client with a certain nickname. (gets a lock to the clients list during execution) */
connected_client *server::l_get_client_ref(std::string &nickname) {

    connected_client *target = nullptr;

    // --------------------------------------------------------------------------------------------------------------------------------------------------
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_clients.lock();
    // ENTER CRITICAL REGION =======================================
    /* Reading the list can cause problems if some thread is trying to add or remove a client,
    thus a semaphore is used. */
    // Searches for the client in the list.
    for(auto it = this->clients.begin(); it != this->clients.end(); it++) {
        if((*it)->l_get_nickname().compare(nickname) == 0) {
            target = (*it);
            break;
        }
    }
    // EXIT CRITICAL REGION ========================================
    // Exits the critical region, and opens the semaphore.
    this->updating_clients.unlock();
    // --------------------------------------------------------------------------------------------------------------------------------------------------

    // Returns a pointer to the client or nullptr if no client was found.
    return target;

}

// ==============================================================================================================================================================
// Requests =====================================================================================================================================================
// ==============================================================================================================================================================

/* Tries changing the nickname of a certain client */
void server::nickname_request(connected_client *client, std::string &nickname) {

    // Checks if the nickname doesn't exist on the server.
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    if(this->l_get_client_ref(nickname) != nullptr)
         client->l_spawn_send_message_worker(new std::string("server: this nickname already exists!"));

    bool success = client->l_set_nickname(nickname); // Tries updating the nickname. 

    // Sends a message to the client telling the results.
    if(success)
        client->l_spawn_send_message_worker(new std::string("server: your nickname was changed to " + nickname + "!"));
    else
        client->l_spawn_send_message_worker(new std::string("server: this nickname is invalid! (It can't start with '#' or '&' and must not contain spaces or commas)"));

}
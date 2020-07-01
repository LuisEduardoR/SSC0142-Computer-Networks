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

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the server should be closed.
bool CLOSE_SERVER_FLAG = false;

// Sets the flag to indicate the server should be closed.
void close_server(int signal_num) { CLOSE_SERVER_FLAG = true; }

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
    for(auto it = this->client_connections.begin(); it != this->client_connections.end(); it++) {

        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        (*it)->updating.lock();

        // ENTER CRITICAL REGION =======================================

        (*it)->kill = true; // Marks that this clients is to be killed.

        // EXIT CRITICAL REGION ========================================

        // Exits the critical region, and opens the semaphore.
        (*it)->updating.unlock();

    }

    // Joins the threads before exiting.
    for(auto it = this->connection_threads.begin(); it != this->connection_threads.end(); it++)
        (*it).join();

    // Clears the memory of any remaining channels.
    for(auto it = this->channels.begin(); it != this->channels.end(); it++)
        delete (*it);

}

// Handles the server instance (control of the program is given to the server until it finishes).
void server::handle() {

    // Sets the server to be closed when CTRL+C is pressed.
    std::signal(SIGINT, close_server);

    // Creates the default idle channel.
    this->create_channel("idle");

    // Creates a thread to handle new connections.
    std::thread check_connections_thread(&server::t_check_for_connections, this);

    // Waits for the check for conenctions thread to finish.
    check_connections_thread.join();

}

// Used as a thread to check for connecting clients.
void server::t_check_for_connections() {

    while(!CLOSE_SERVER_FLAG)
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

        // Initializes the new connection.
        // Handles creating the new connection and adding it to the array in a thread safe way. ----------------------------------------------------------V

        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating_connections.lock();

        // ENTER CRITICAL REGION =======================================

        std::cerr << "Client just connected with socket " << new_connection->client_socket << "!" << std::endl;

        // Creates a new thread to handle the connection.
        this->connection_threads.push_back(std::thread(&connected_client::t_handle, new_connection));

        // Adds the new connection to the list, modifying the list can cause problems if some client handler is reading it at the same time, thus a semaphore is used.
        this->client_connections.push_back(new_connection);

        // Adds the new client to the idle channel.
        this->channels[0]->add_client(new_connection);

        // EXIT CRITICAL REGION ========================================

        // Exits the critical region, and opens the semaphore.
        this->updating_connections.unlock();

        // --------------------------------------------------------------------------------------------------------------------------------------------------

    }

}

// Removes a client that has disconnected from the server.
void server::remove_client(connected_client *connection) {

    // Removes the client from it's channel.
    int client_channel = connection->get_channel();
    if(client_channel >= 0)
        connection->server_instance->channels[client_channel]->remove_client(connection);

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    connection->server_instance->updating_connections.lock();

    // ENTER CRITICAL REGION =======================================

    std::cerr << "Client with socket " << connection->client_socket << " disconnected!" << std::endl;

    // Mark that this connection is being removed.
    connection->kill = true;

    // Finds the connection that is being removed on the list.
    int found_connection = 0;
    for(size_t i = 0; i < connection->server_instance->client_connections.size(); i++)
        if(connection->server_instance->client_connections[i] == connection)
            found_connection = i;

    // Gets an iterator to the connection that needs to be removed and erases it.
    std::vector<connected_client*>::iterator iter = connection->server_instance->client_connections.begin() + found_connection;
    connection->server_instance->client_connections.erase(iter, iter);

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    connection->server_instance->updating_connections.unlock();

    // Ensures that any thread trying to check if a message was successfully sent has ended.
    usleep(2 * ACKNOWLEDGE_WAIT_TIME);
    
    // Deletes the current connection.
    delete connection;

}

bool server::create_channel(std::string name) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_channels.lock();

    // ENTER CRITICAL REGION =======================================

    // Creates the new channel and adds it to the list.
    this->channels.push_back(new channel(this->channels.size(), name, this));

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_channels.unlock();

    std::cerr << "Channel " << name << " created!" << std::endl;

    return true;

}
// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "server.hpp"
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

// Creates a new server with a network socket and binds the socket.
server::server(int port_number) { 

    // Creates a TCP socket.
    this->server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Sets the socket to be non-blocking.
    int flags = fcntl(this->server_socket, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(this->server_socket, F_SETFL, flags);

    // Gets an adress for the socket.
    this->server_adress.sin_family = AF_INET;
    this->server_adress.sin_port = htons(port_number);
    this->server_adress.sin_addr.s_addr = INADDR_ANY;

    // Binds the server to the socket.
    this->server_status = bind(this->server_socket, (struct sockaddr *) &(this->server_adress), sizeof(this->server_adress));

    // Inializes the client connections list.
    client_connections.clear();

}

// DESTRUCTOR
server::~server() { 

    // Closes the socket.
    close(this->server_socket);

    // Joins the threads before exiting.
    for(auto it = this->connection_threads.begin(); it != this->connection_threads.end(); it++)
        (*it).join();

    // Clears the memory of any remaining channels.
    for(auto it = this->channels.begin(); it != this->channels.end(); it++)
        delete (*it);

}

// CONSTRUCTOR
client_connection::client_connection(int socket, server *server_instance) {

    this->alive = 1;
    this->client_socket = socket;
    this->server_instance = server_instance;

    // Initially the nickname comes fron the client socket.
    this->nickname = "socket " + std::to_string(socket);

    // Initially all clients have no channel (but should be put on the idle channel after being created).
    this->channel = -1;

}

// Used to pass information about the message that needs to be redirected to the threads.
class redirect_message
{

    public:

        redirect_message(int max_resending_attempts, client_connection *target_client, std::string message);

        int attempts;
        client_connection *target_client;
        std::string message;

    private:

};

// CONSTRUCTOR
redirect_message::redirect_message(int max_resending_attempts, client_connection *target_client, std::string message) {

    this->attempts = max_resending_attempts;
    this->target_client = target_client;
    this->message = message;

}

// Private function headers, will be declared bellow. ====================================================================
void handle_client(client_connection *client_connect);
void handle_connections(server *serv);
void close_server(int signal_num);
void remove_client(client_connection *connection);
void create_send_message_worker(client_connection *target_socket, std::string message);
void send_message_worker(redirect_message *redirect);
// =======================================================================================================================

// Handles the server instance (control of the program is given to the server until it finishes).
void server::handle() {

    // Sets the server to be closed when CTRL+C is pressed.
    std::signal(SIGINT, close_server);

    // Creates the default idle channel.
    this->create_channel("idle");

    // Creates a thread to handle new connections.
    std::thread connection_thread(handle_connections, this);

    // Waits for the connection thread to finish.
    connection_thread.join();

}

void handle_connections(server *serv) {

    while(!CLOSE_SERVER_FLAG)
    {

        // Listens for new connection.
        int status = 0;
        client_connection *new_connection = serv->listen_for_connections(&status);

        if(new_connection == nullptr) {

            if(status != 1) // If status is 1, no new connection is currently avaliable, otherwise an error ocurred.
                std::cerr << "Unidentified connection error!" << std::endl;

        } else { // Initializes the new connection.

            // Handles creating the new connection and adding it to the array in a thread safe way. ----------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            serv->updating_connections.lock();

            // ENTER CRITICAL REGION =======================================

            std::cerr << "Client just connected with socket " << new_connection->client_socket << "!" << std::endl;

            // Creates a new thread to handle the connection.
            serv->connection_threads.push_back(std::thread(handle_client, new_connection));

            // Adds the new connection to the list, modifying the list can cause problems if some client handler is reading it at the same time, thus a semaphore is used.
            serv->client_connections.push_back(new_connection);

            // Adds the new client to the idle channel.
            serv->channels[0]->add_client(new_connection);

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            serv->updating_connections.unlock();

            // --------------------------------------------------------------------------------------------------------------------------------------------------

        }
    }

}

void handle_client(client_connection *client_connect) {

    while(!CLOSE_SERVER_FLAG) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string received_message = check_message(client_connect->client_socket, &status, 0);
        
        // Redirects the message to the other clients.
        if(status == 0) { // A message was received and must be redirected.

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            client_connect->server_instance->updating_connections.lock();

            // ENTER CRITICAL REGION =======================================

            if(received_message.compare("/ping") == 0) { // Detects the ping command.

                // Creates the string to be sent.
                std::string sending_string = std::string("server: pong");

                // Creates the worker thread to send the message.
                create_send_message_worker(client_connect, sending_string);

            } else if(received_message.compare(ACKNOWLEDGE_MESSAGE) == 0) { // Detects that the client is confirming a received message.

                // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
                client_connect->updating_client_connection.lock();

                // ENTER CRITICAL REGION =======================================

                client_connect->ack_received_message--; // Marks that the client has acknowledge a message. 

                // EXIT CRITICAL REGION ========================================

                // Exits the critical region, and opens the semaphore.
                client_connect->updating_client_connection.unlock();


            } else if (received_message.substr(0,6).compare("/send ") == 0) { // Regular message that needs to be sent to others.

                // Creates a string to be sent, in the format [CHANNEL] NICKNAME:MESSAGE.
                std::string sending_string =  "[" + client_connect->server_instance->channels[client_connect->channel]->name + "] ";
                sending_string += client_connect->nickname + ": " + received_message.substr(6,received_message.length());

                // Sends the message to all the other clients on the channel. Reading this list could cause problems if a new connection is being added or removed, thus a semaphore is used.
                for(auto it = client_connect->server_instance->channels[client_connect->channel]->members.begin(); 
                    it != client_connect->server_instance->channels[client_connect->channel]->members.end();
                    it++) {

                    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
                    client_connect->server_instance->channels[client_connect->channel]->updating_members.lock();

                    // ENTER CRITICAL REGION =======================================

                    // Redirects the message to all clients in the channel.
                    create_send_message_worker(*it, sending_string);

                    // EXIT CRITICAL REGION ========================================

                    // Exits the critical region, and opens the semaphore.
                    client_connect->server_instance->channels[client_connect->channel]->updating_members.unlock();

                }

            }

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            client_connect->server_instance->updating_connections.unlock();

        } else if(status == 1) { // No new messages from this client.
            // If there are no messages nothing is done.
        } else if(status == -1) { // The client has disconnected.
            break; // Exits the thread.
        } else { // An error has happened.
            std::cerr << "ERROR " << status << "!" << std::endl;
        }
        
    }

    // Disconnects the client before exiting this thread.
    remove_client(client_connect);

}

// Creates a worker thread to send a message.
void create_send_message_worker(client_connection *target_client, std::string message) {

    // Creates a structure to contain the data necessary for the worker thread.
    redirect_message *redirect = new redirect_message(MAX_RESENDING_ATTEMPS, target_client, message);

    // Creates the worker thread.
    std::thread worker(send_message_worker, redirect);
    worker.detach();

}

// Used as a worker thread to redirect messages to all other clients at the same time.
void send_message_worker(redirect_message *redirect) {

    // Gets the information about the redirected message.
    redirect_message *redirected_message = (redirect_message*)redirect;

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    redirected_message->target_client->updating_client_connection.lock();

    // ENTER CRITICAL REGION =======================================

    redirected_message->target_client->ack_received_message++; // Marks that this clients needs to acknwoledge that a message has being received.

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    redirected_message->target_client->updating_client_connection.unlock();

    // Attempt sending the message.
    int success = 0;
    while (redirected_message->attempts > 0) {

        if(!redirected_message->target_client->alive) // Checks if the client is still connected.
            break;

        // Sends the message.
        std::string message = std::string(redirected_message->message);
        send_message(redirected_message->target_client->client_socket, message);

        // Sleeps the thread for the desired time to wait for a response.
        usleep(ACKNOWLEDGE_WAIT_TIME);

        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        redirected_message->target_client->updating_client_connection.lock();

        // ENTER CRITICAL REGION =======================================

        if(redirected_message->target_client->ack_received_message < 1) // Marks that all messages were successfully sent.
            success = 1;

        // EXIT CRITICAL REGION ========================================

        // Exits the critical region, and opens the semaphore.
        redirected_message->target_client->updating_client_connection.unlock();

        // Breaks when the message was successfully sent.
        if(success)
            break;

        // If the message failed to be sent, decreases the number of attemps remaining.
        redirected_message->attempts--;

    }

    // If the client could not confirm the message was received, shut it down.
    if(!success && redirected_message->target_client->alive)
        shutdown(redirected_message->target_client->client_socket, 2);

    // Deletes the struct with the redirection information and the stored message.
    delete redirected_message;
    
}

// Listen and accept incoming connections, return the incoming client's connection.
client_connection *server::listen_for_connections(int *status) {

    // Listens to the socket.
    listen(this->server_socket, BACKLOG_LEN);

    // Accepts the connection and returns the client socket.
    int new_client_socket = accept(this->server_socket, nullptr, nullptr);

    // If no connection happened, checks why.
    if(new_client_socket == -1) {

        if(errno == EAGAIN || errno == EWOULDBLOCK) { // No connection is avaliable.
            *status = 1;
        } else { // Error.
            *status = -1;
        }

        return NULL;
    }

    // Creates a new connection object and assigns the socket.
    client_connection *new_connection = new client_connection(new_client_socket, this);

    // Marks that there has been a new connection.
    *status = 0;

    return new_connection;
}

// Removes a client that has disconnected from the server.
void remove_client(client_connection *connection) {

    // Removes the client from it's channel.
    if(connection->channel >= 0)
        connection->server_instance->channels[connection->channel]->remove_client(connection);

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    connection->server_instance->updating_connections.lock();

    // ENTER CRITICAL REGION =======================================

    std::cerr << "Client with socket " << connection->client_socket << " disconnected!" << std::endl;

    // Mark that this connection is being removed.
    connection->alive = 0;

    // Finds the connection that is being removed on the list.
    int found_connection = 0;
    for(size_t i = 0; i < connection->server_instance->client_connections.size(); i++)
        if(connection->server_instance->client_connections[i] == connection)
            found_connection = i;

    // Gets an iterator to the connection that needs to be removed and erases it.
    std::vector<client_connection*>::iterator iter = connection->server_instance->client_connections.begin() + found_connection;
    connection->server_instance->client_connections.erase(iter, iter);

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    connection->server_instance->updating_connections.unlock();

    // Ensures that any thread trying to check if a message was succesfully sent has ended.
    usleep(2 * ACKNOWLEDGE_WAIT_TIME);
    

    // Deletes the current connection.
    delete connection;

}

// Sets the flag to indicate the server should be closed.
void close_server(int signal_num) { CLOSE_SERVER_FLAG = true; }

// ======================================================================================================================

//    Channel handling

// ======================================================================================================================

bool server::create_channel(std::string name) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_channels.lock();

    // ENTER CRITICAL REGION =======================================

    // Creates the new channel and adds it to the list.
    this->channels.push_back(new channel(this->channels.size(), name));

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_channels.unlock();

    std::cerr << "Channel " << name << " created!" << std::endl;

    return true;

}

// Creates a channel with an index and a name.
channel::channel(int index, std::string name) {

    this->index = index;
    this->name = name;

}

bool channel::add_client(client_connection *client) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================

    // Adds client to the channel.
    this->members.insert(client);
    
    
    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    client->updating_client_connection.lock();

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
    client->updating_client_connection.unlock();

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    std::cerr << "Client with socket " << client->client_socket << " is now on channel " << this-> name << "! ";
    std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;

    return true;

}

bool channel::remove_client(client_connection *client) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating_members.lock();

    // ENTER CRITICAL REGION =======================================

    // Adds client to the channel.
    this->members.erase(client);

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    client->updating_client_connection.lock();

        // ENTER CRITICAL REGION =======================================

        // Sets the client channel to none (note that if the client exited the channel orderly it should be sent back to the idle channel).
        client->channel = -1;

        // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    client->updating_client_connection.unlock();
    

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating_members.unlock();

    std::cerr << "Client with socket " << client->client_socket << " left channel " << this-> name << "! ";
    std::cerr << "(Channel members: " << std::to_string(this->members.size()) << ")" << std::endl;

    return true;

}
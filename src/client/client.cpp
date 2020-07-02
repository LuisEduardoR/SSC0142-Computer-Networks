// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client.hpp"
# include "../messaging/messaging.hpp"

# include <iostream>
# include <string>

# include <mutex>
# include <thread>
# include <atomic>

# include <errno.h>

# include <fcntl.h>
# include <csignal>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the client should be closed.
std::atomic_bool close_client_flag(false);

// Used to make the client don't close on a CTRL + C;
void ignore_sigint(int signal_num) {

    // Prints a message and resets the signal.
    std::cout <<  std::endl << "Use the /quit command or input EOF (CTRL+D) to exit!" << std::endl;
    std::signal(SIGINT, ignore_sigint);

}

// CONSTRUCTOR
client::client() { 

    // Creates a TCP socket.
    this->network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initializes the client new messages as empty.
    this->new_messages.clear();

}

// DESTRUCTOR
client::~client() {

    // Closes the socket.
    close(this->network_socket);
}

// Tries connecting to a server and returns the connection status.
int client::connect_to_server(const char *s_addr, int port_number){

    // Gets an address for the socket.
    this->server_address.sin_family = AF_INET;
    this->server_address.sin_port = htons(port_number);
    inet_pton(AF_INET,  s_addr, &(this->server_address.sin_addr));

    // Connects to the server.
    this->connection_status = connect(this->network_socket, (struct sockaddr *) &(this->server_address), sizeof(this->server_address));

    // Returns the status of the connection.
    return this->connection_status;

}

// Handles the client instance (control of the program is given to the client until it disconnects).
void client::handle() {

    // Sets the client to ignore SIGINT displaying a mesage instead.
    std::signal(SIGINT, ignore_sigint);

    // Creates a thread to constantly check for new messages.
    std::thread server_listen_thread(&client::t_listen_to_server, this);
    
    // Store commands received.
    std::string command_buffer;

    // Asks for a initial nickname on the server.
    std::cout << std::endl << "Enter a nickname:" << std::endl << std::endl;

    // Receives the nickname.
    std::getline(std::cin, command_buffer);

    
    // Sends the nickname to the server.
    command_buffer = "/nickname " + command_buffer;
    send_message(this->network_socket, command_buffer);
    std::cout << std::endl << "Nickname sent to server... Use /new to check for the server response! If your nickname is invalid you will be given a default nickname that can be changed later!" << std::endl;

    do {

        // Exits on end-of-file.
        if(std::cin.eof()) {
            close_client_flag = true;
            continue;
        }

        // Prints commands.
        std::cout << std::endl << "Enter a command:" << std::endl << std::endl;
        std::cout << "\t/join\t\t<CHANNEL NAME>\t- Joins a channel" << std::endl;
        std::cout << "\t/nickname\t<NICKNAME>\t- Change your nickname" << std::endl;
        std::cout << "\t/send\t\t<MESSAGE>\t- Send a message" << std::endl;
        std::cout << "\t/ping\t\t\t\t- The server answers \"pong\"" << std::endl;
        std::cout << "\t/quit\t\t\t\t- Close the connection and exit the program" << std::endl << std::endl;

        // Receives commands.
        std::getline(std::cin, command_buffer);

        if(command_buffer.substr(0,4).compare("/new") == 0 && command_buffer.length() == std::string("/new").length()) {

            std::cout << std::endl << "Checking for new messages..." << std::endl << std::endl;
            // Handles showing the new messages in a thread safe way. -------------------------------------------------------------------------------------------
            this->updating_messages.lock(); // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            // ENTER CRITICAL REGION ============================================================================================================================
            show_new_messages();
            // EXIT CRITICAL REGION =============================================================================================================================
            this->updating_messages.unlock(); // Exits the critical region, and opens the semaphore.
            // --------------------------------------------------------------------------------------------------------------------------------------------------
            continue;

        }

        // Checks for the quit command.
        if(command_buffer.substr(0,5).compare("/quit") == 0 && command_buffer.length() == std::string("/quit").length()) {
            close_client_flag = true;
            continue;
        }

        // Sends all other messages to the server.
        send_message(this->network_socket, command_buffer);

        // Prints a message for the /send command
        if(command_buffer.substr(0,6).compare("/send ") == 0) {
            std::cout << std::endl << "Message sent to server..." << std::endl;
            continue;
        }

        // Prints a message for the /ping command
        if(command_buffer.substr(0,5).compare("/ping") == 0 && command_buffer.length() == std::string("/ping").length()) {
            std::cout << std::endl << "Ping sent to server... Use /new to check for the server response!" << std::endl;
            continue;
        }

        // Prints a message for the /nickname command
         if(command_buffer.substr(0,10).compare("/nickname ") == 0) {
            std::cout << std::endl << "Trying to change nickname... Use /new to check for the server response!" << std::endl;
            continue;
        }

        // Prints a message for the /join command
         if(command_buffer.substr(0,6).compare("/join ") == 0) {
            std::cout << std::endl << "Trying to join channel... Use /new to check for the server response!" << std::endl;
            continue;
        }

    } while (!close_client_flag);

    // Joins the thead listening for messages message thread before returning.
    server_listen_thread.join();

}

// Handles listening for the server on a separate thread.
void client::t_listen_to_server() {

    while (!close_client_flag) {    

        // Receives data from the server. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string response_message = check_message(this->network_socket, &status, 1);

        if(status == 0) {

            // Handles showing the new messages in a thread safe way. ------------------------------------------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            this->updating_messages.lock();

            // Transfers the response buffer to the new message list.
            this->new_messages.push_back(response_message);

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            this->updating_messages.unlock();

            // --------------------------------------------------------------------------------------------------------------------------------------------------

        } else if (status == 1) { // No new messages.
            // Do nothing.
        } else { // Server was lost.
            close_client_flag = true; // Sets the client to close.
        }
    }

}

// Shows client new messages.
void client::show_new_messages() {

    if(this->new_messages.size() == 0)
        std::cout << "No new messages available!" << std::endl;
    else {

        std::cout << "You have new messages:" << std::endl << std::endl;

        // Prints the messages.
        for(size_t i = 0; i < this->new_messages.size(); i++)
            std::cout << "- " << this->new_messages[i] << std::endl;

        // Empties the new message array.
        this->new_messages.clear();

    }

}


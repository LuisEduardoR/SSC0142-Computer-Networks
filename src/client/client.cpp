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

# include <errno.h>

# include <fcntl.h>
# include <csignal>

# include <sys/types.h>
# include <sys/socket.h>

# include <arpa/inet.h>
# include <netinet/in.h>

#include <unistd.h>

// Used to indicate when the client should be closed.
int CLOSE_CLIENT_FLAG = 0;

// Private function headers, will be declared bellow. ====================================================================
void client_check_message(client *current_client);
void show_new_messages(client *c);
// =======================================================================================================================

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

    // Gets an adress for the socket.
    this->server_adress.sin_family = AF_INET;
    this->server_adress.sin_port = htons(port_number);
    inet_pton(AF_INET,  s_addr, &(this->server_adress.sin_addr));

    // Connects to the server.
    this->connection_status = connect(this->network_socket, (struct sockaddr *) &(this->server_adress), sizeof(this->server_adress));

    // Returns the status of the connection.
    return this->connection_status;

}

// Handles the client instance (control of the program is given to the client until it disconnects).
void client::handle() {

    // Sets the client to ignore SIGINT displaying a mesage instead.
    std::signal(SIGINT, ignore_sigint);

    // Creates a thread to constantly check for new messages.
    std::thread check_message_thread(client_check_message, this);
    
    // Store commands received.
    std::string command_buffer;

    do {

        // Exits on end-of-file.
        if(std::cin.eof()) {
            CLOSE_CLIENT_FLAG = 1;
            continue;
        }

        // Prints commands.
        std::cout << std::endl << "Enter a command:" << std::endl << std::endl;
        std::cout << "\t/new\t-\tShows any new messages" << std::endl;
        std::cout << "\t/send\t-\tSend a message" << std::endl;
        std::cout << "\t/ping\t-\tThe server answers \"pong\"" << std::endl;
        std::cout << "\t/quit\t-\tClose the connection and exit the program" << std::endl << std::endl;

        // Receives commands.
        std::cin >> command_buffer;

        if(command_buffer.compare("/new") == 0) {

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

        if(command_buffer.compare("/send") == 0) {

            std::cout << std::endl << "Enter the message:" << std::endl << std::endl;

            // Receives the message to be sent to the server. A buffer with appropriate size is allocated and must be freed later!
            std::getc(stdin); // Throws alway the '\n' from the ENTER.
            std::getline(std::cin, command_buffer);

            // Checks if the message is valid.
            if(command_buffer[0] == '/') {
                std::cout << std::endl << "A message can't start with '/'!" << std::endl;
            } else {

                // Sends the message to the server to be redirected to the other clients.
                int status;
                send_message(this->network_socket, command_buffer);
                std::cout << std::endl << "Message sent to server..." << std::endl;

            }

            continue;

        }


        if(command_buffer.compare("/ping") == 0) {

            // Sends the the /ping command to the client, use /new to see if "pong" was received.
            int status;
            std::string ping("/ping");
            send_message(this->network_socket, ping);
            std::cout << std::endl << "Ping sent to server... Use /new to check for the response!" << std::endl;

            continue;

        }

        if(command_buffer.compare("/quit") == 0) {
            CLOSE_CLIENT_FLAG = 1;
            continue;
        }

    } while (!CLOSE_CLIENT_FLAG);

    // Joins the new message thread before returning.
    check_message_thread.join();

}

// Handles checking the client messages on a separate thread.
void client_check_message(client *current_client) {

    while (!CLOSE_CLIENT_FLAG) {    

        // Receives data from the server. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string response_message = check_message(current_client->network_socket, &status, 1);

        if(status == 0) {

            // Handles showing the new messages in a thread safe way. ------------------------------------------------------------------------------------------V

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            current_client->updating_messages.lock();

            // Transfers the response buffer to the new message list.
            current_client->new_messages.push_back(response_message);

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            current_client->updating_messages.unlock();

            // --------------------------------------------------------------------------------------------------------------------------------------------------

        } else if (status == 1) { // No new messages.
            // Do nothing.
        } else { // Server was lost.
            CLOSE_CLIENT_FLAG = 1; // Sets the client to close.
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
        for(int i = 0; i < this->new_messages.size(); i++)
            std::cout << "- " << this->new_messages[i] << std::endl;

        // Empties the new message array.
        this->new_messages.clear();

    }

}


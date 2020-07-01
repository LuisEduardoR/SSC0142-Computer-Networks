// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "main_server.hpp"
# include "connected_client.hpp"
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

// CONSTRUCTOR
connected_client::connected_client(int socket, server *server_instance) {

    this->kill = false;
    this->client_socket = socket;
    this->server_instance = server_instance;
    // Initially the nickname comes from the client socket.
    this->nickname = "socket " + std::to_string(socket);
    // Initially all clients have no channel (but should be put on the idle channel after being created).
    this->channel = -1;

}

// Thread that handles the client connection to the server.
void connected_client::t_handle() {

    while(!this->kill) {

        // Checks for data from the client. A buffer with appropriate size is allocated and must be freed later!
        int status = 0;
        std::string received_message = check_message(this->client_socket, &status, 0);
        
        // Redirects the message to the other clients.
        if(status == 0) { // A message was received and must be redirected.

            // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
            this->server_instance->updating_connections.lock();

            // ENTER CRITICAL REGION =======================================

            if(received_message.compare("/ping") == 0) { // Detects the ping command.

                // Creates the string to be sent.
                std::string sending_string = std::string("server: pong");

                // Creates the worker thread to redirect (send) the message.
                this->redirect_message(sending_string);

            } else if(received_message.compare(ACKNOWLEDGE_MESSAGE) == 0) { // Detects that the client is confirming a received message.

                // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
                this->updating.lock();

                // ENTER CRITICAL REGION =======================================

                this->ack_received_message--; // Marks that the client has acknowledge a message. 

                // EXIT CRITICAL REGION ========================================

                // Exits the critical region, and opens the semaphore.
                this->updating.unlock();


            } else if (received_message.substr(0,6).compare("/send ") == 0) { // Regular message that needs to be sent to others.

                // Creates a string to be sent, in the format [CHANNEL] NICKNAME:MESSAGE.
                std::string sending_string =  "[" + this->server_instance->channels[this->channel]->name + "] ";
                sending_string += this->nickname + ": " + received_message.substr(6,received_message.length());

                // Sends the message to all the other clients on the channel. Reading this list could cause problems if a new connection is being added or removed, thus a semaphore is used.
                for(auto it = this->server_instance->channels[this->channel]->members.begin(); 
                    it != this->server_instance->channels[this->channel]->members.end();
                    it++) {

                    // Redirects the message to all clients in the channel.
                   (*it)->redirect_message(sending_string);

                }

            }

            // EXIT CRITICAL REGION ========================================

            // Exits the critical region, and opens the semaphore.
            this->server_instance->updating_connections.unlock();

        } else if(status == 1) { // No new messages from this client.
            // If there are no messages nothing is done.
        } else if(status == -1) { // The client has disconnected.
            break; // Exits the thread.
        } else { // An error has happened.
            std::cerr << "ERROR " << status << "!" << std::endl;
        }
        
    }

    // Disconnects the client before exiting this thread.
    this->server_instance->remove_client(this);

}

// CONSTRUCTOR
redirected_message::redirected_message(int max_resending_attempts, std::string message) {

    this->attempts = max_resending_attempts;
    this->message = message;

}

// Used by the server to redirect a message to acertain client.
void connected_client::redirect_message(std::string message) {

    // Creates a structure to contain the data necessary for the worker thread.
    redirected_message *redirect = new redirected_message(MAX_RESENDING_ATTEMPS, message);

    // Creates the worker thread.
    std::thread worker(&connected_client::t_redirect_message_worker, this, redirect);
    worker.detach();

}

// Used as a worker thread to redirect messages to a client and check if the client received the message.
void connected_client::t_redirect_message_worker(redirected_message *redirect) {

    // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
    this->updating.lock();

    // ENTER CRITICAL REGION =======================================

    this->ack_received_message++; // Marks that this clients needs to acknowledges that a message has being received.

    // EXIT CRITICAL REGION ========================================

    // Exits the critical region, and opens the semaphore.
    this->updating.unlock();

    // Attempt sending the message.
    int success = 0;
    while (redirect->attempts > 0) {

        if(this->kill) // Checks if the client is still connected.
            break;

        // Sends the message.
        std::string message = std::string(redirect->message);
        send_message(this->client_socket, message);

        // Sleeps the thread for the desired time to wait for a response.
        usleep(ACKNOWLEDGE_WAIT_TIME);

        // Waits for the semaphore if necessary, and enters the critical region, closing the semaphore.
        this->updating.lock();

        // ENTER CRITICAL REGION =======================================

        if(this->ack_received_message < 1) // Marks that all messages were successfully sent.
            success = 1;

        // EXIT CRITICAL REGION ========================================

        // Exits the critical region, and opens the semaphore.
        this->updating.unlock();

        // Breaks when the message was successfully sent.
        if(success)
            break;

        // If the message failed to be sent, decreases the number of attemps remaining.
        redirect->attempts--;

    }

    // If the client could not confirm the message was received, shut it down.
    if(!success && !this->kill)
        shutdown(this->client_socket, 2);

    // Deletes the struct with the redirection information and the stored message.
    delete redirect;
    
}
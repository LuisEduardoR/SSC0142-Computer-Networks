// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "client/client.hpp"
# include "server/main_server.hpp"
# include "messaging/messaging.hpp"

# include <iostream>
# include <string>

// Default values.
# define DEFAULT_ADDR "127.0.0.1"
# define DEFAULT_PORT 9002

// Help texts.
# define HELP_NO_PARAMETERS "\nusage: ./trabalho-redes [parameters]\n\nFor a list of parameters type \"./trabalho-redes --help\"\n"
# define HELP_FULL "\nusage: ./trabalho-redes PARAMETERS\n\nYou can choose to connect as a client or as a server.\n\n\tTo connect as a client use:\n\t\t./trabalho-redes client\n\n\tTo connect as a server use:\n\t\t./trabalho-redes server (For default port)\n\t\t\tor\n\t\t./trabalho-redes server [port]\n"
# define HELP_CLIENT "\nusage:\n./trabalho-redes client\n"
# define HELP_SERVER "\nusage:\n./trabalho-redes server (For default port)\n\tor\n./trabalho-redes server [port]\n"

// Types of program instances.
enum TYPE { CLIENT, SERVER };

// Program main function.
int main(int argc, char* argv[])
{

    // Displays help text, if wrong number of parameters was passed.
    if(argc < 2) {
        std::cout << HELP_NO_PARAMETERS << std::endl;
        return 0;
    }

    // Gets the argument with index 1.
    std::string argv_1(argv[1]);

    // Displays help text, if asked to.
    if(argv_1.compare("--help") == 0) {
        std::cout << HELP_FULL << std::endl;
        return 0;
    }

    // Checks for the type to be used for this process instance.
    enum TYPE instance_type;
    if(argv_1.compare("client") == 0) {
        instance_type = CLIENT;
    } else if(argv_1.compare("server") == 0) {
        instance_type = SERVER;
    } else {
        std::cout << HELP_FULL << std::endl;
        return 0;
    }

    if(instance_type == CLIENT) { // Runs for the client.

        // Stores the IP address and port of ther server to connect.
        std::string server_addr;
        int server_port;

        // Checks for the client parameters.
        if(argc != 2) { // If unnecessary parameters were provided use prints a help message.
            std::cout << HELP_CLIENT << std::endl;
            return 0;
        }

        // Store commands received.
        std::string command_buffer;

        // Receives the commands for the client.
        do {

            // Exits on end-of-file.
            if(std::cin.eof()) {
                std::cout << std::endl;
                return 0;
            }

            // Prints options.
            std::cout << std::endl << "Enter a command:" << std::endl << std::endl;
            std::cout << "\t/connect\t-\tConnect to a server" << std::endl;
            std::cout << "\t/quit\t\t-\tExit the program" << std::endl << std::endl;
            
            // Receives commands.
            std::getline(std::cin, command_buffer);

            // Start the server connection process.
            if(command_buffer.compare("/connect") == 0) {
                
                std::cout << std::endl << "Enter the server address (default: " << DEFAULT_ADDR << ")" << std::endl << std::endl;
                std::getline(std::cin, command_buffer);

                if(command_buffer.compare("default") == 0) // Uses the default address.
                    server_addr = DEFAULT_ADDR;
                else // Uses the provided address.
                    server_addr = command_buffer;

                std::cout << std::endl << "Enter the server port (default: " << DEFAULT_PORT << ")" << std::endl << std::endl;
                std::getline(std::cin, command_buffer);

                if(command_buffer.compare("default") == 0) // Uses the default port.
                    server_port = DEFAULT_PORT;
                else // Uses the provided port.
                    server_port =  std::stoi(command_buffer);

                // Creates the client.
                std::cout << std::endl << "Creating new client..." << std::endl;
                client *c = new client();
                // Checks for errors creating the client.
                if(c == nullptr) {
                    std::cerr << "Error creating new client! (-1)" << std::endl << std::endl;
                    return -1;
                }
                std::cout << "Client created successfully!" << std::endl;

                // Connects to the server.
                std::cout << std::endl << "Attempting connection to server (" << server_addr << ":" << server_port << ")..." << std::endl;
                int cnct_status = c->connect_to_server(server_addr.c_str(), server_port);
                // Checks for errors.
                if(cnct_status < 0) {
                    std::cerr << "Error connecting to remote socket! (" << cnct_status << ")" << std::endl << std::endl;
                    delete c;
                    return cnct_status;
                }
                std::cout << "Connected to the server successfully!" << std::endl;

                // Handles the client execution until it's disconnected.
                c->handle();

                // Deletes the client.
                std::cout << std::endl << "Disconnected from the server!" << std::endl << std::endl;
                delete c;

                // Exits the program.
                return 0;
            }

            if(command_buffer.compare("/quit") == 0)
                return 0;

        } while (true);

    } else if(instance_type == SERVER) { // Handles the server.

        // Stores the port where the server will be hosted.
        int server_port;

        // Checks for the client parameters.
        if(argc == 2) // If no additional parameters were provided use the default ones.
            server_port = DEFAULT_PORT;
        else if (argc == 3) // If a port is provided use it instead.
            server_port = std::stoi(std::string(argv[2]));
        else { // Displays help text if the number of parameters is invalid.
            std::cout << HELP_SERVER << std::endl;
            return 0;
        }

        // Creates the server.
        std::cout << std::endl << "Creating server at port " << server_port << "..." << std::endl;
        server *s = new server(server_port);
        // Checks for errors creating the server.
        if(s == nullptr) {
            std::cerr << "Error creating server! (-1)" << std::endl << std::endl;
            return -1;
        }
        
        // Checks for errors.
        int svr_status = s->server_status;
        if(svr_status < 0) {
            std::cerr << "Error connecting to socket! (" << svr_status << ")" << std::endl << std::endl;
            delete s;
            return svr_status;
        }
        std::cout << "Server created successfully!" << std::endl << std::endl;

        // Handles the server execution.
        std::cout << "Running... (Press CTRL+C to stop)" << std::endl << std::endl;
        s->handle();

        // Deletes the server after it's done.
        std::cout << std::endl << "Server closed!" << std::endl << std::endl;;
        delete s;

    } else // Don't do anything for other parameters.
        std::cout << std::endl << "Invalid parameter!" << std::endl << std::endl;

    return 0;

}
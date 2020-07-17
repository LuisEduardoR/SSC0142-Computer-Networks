// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "color.hpp"

# include "messaging.hpp"
# include "client/client.hpp"
# include "server/main_server.hpp"

# include <iostream>
# include <string>

// Help texts.
# define HELP_NO_PARAMETERS "\nusage: ./trabalho-redes [parameters]\n\nFor a list of parameters type \"./trabalho-redes --help\"\n"
# define HELP_FULL "\nusage: ./trabalho-redes PARAMETERS\n\nYou can choose to connect as a client or as a server.\n\n\tTo connect as a client use:\n\t\t./trabalho-redes client\n\n\tTo connect as a server use:\n\t\t./trabalho-redes server (For default port)\n\t\t\tor\n\t\t./trabalho-redes server [port]\n"
# define HELP_CLIENT "\nusage:\n./trabalho-redes client\n"
# define HELP_SERVER "\nusage:\n./trabalho-redes server (For default port)\n\tor\n./trabalho-redes server [port]\n"

// Default address value.
constexpr char default_addr[] = "127.0.0.1";

// Default port value.
constexpr int default_port = 9002;

// Types of program instances.
enum instance_type { it_Invalid, it_Client, it_Server };

// Program main function.
int main(int argc, char* argv[])
{

    // Displays help text, if wrong number of parameters was passed.
    if(argc < 2) {
        std::cout << HELP_NO_PARAMETERS << std::endl;
        return 0; // Closes the program after showing the text.
    }

    // Gets the argument with index 1.
    std::string argv_1(argv[1]);

    // Displays help text, if asked to.
    if(argv_1.compare("--help") == 0) {
        std::cout << HELP_FULL << std::endl;
        return 0; // Closes the program after showing the text.
    }

    // Checks for the type to be used for this process instance.
    enum instance_type inst_type = it_Invalid;
    if(argv_1.compare("client") == 0)
        inst_type = it_Client;
    else if(argv_1.compare("server") == 0)
        inst_type = it_Server;
    else
        std::cout << HELP_FULL << std::endl;

    // Handles the client.
    if(inst_type == it_Client) {

        // Checks for the client parameters.
        if(argc != 2) { // If unnecessary parameters were provided prints a help message.
            std::cout << HELP_CLIENT << std::endl;
            return 0;
        }

        // Store commands received.
        std::string command_buffer;

        // Stores the IP address and port of ther server to connect.
        std::string server_addr;
        int server_port;

        // Receives the commands for the client until it connects to a server.
        do {

            // Exits the program on EOF.
            if(std::cin.eof())
                return 0; 

            // Prints options.
            std::cout << std::endl << "Enter a command:" << std::endl << std::endl;
            std::cout << "\t/connect\t-\tConnect to a server" << std::endl;
            std::cout << "\t/quit\t\t-\tExit the program" << std::endl << std::endl;
            
            // Receives commands.
            std::getline(std::cin, command_buffer);

            // Exits the loop and starts the server connection process.
            if(command_buffer.compare("/connect") == 0)
                break;

            // Exits the program without conencting.
            if(command_buffer.compare("/quit") == 0)
                return 0;

        } while (true);

        // Receives the address to attempt connecting to.
        std::cout << std::endl << "Enter the server address (default: " << default_addr << ")" << std::endl << std::endl;
        std::getline(std::cin, command_buffer);
        // Uses the default address if asked to.
        if(command_buffer.compare("default") == 0) 
            server_addr = default_addr;
        else // Uses the provided address.
            server_addr = command_buffer;

        // Receives the port to be used.
        std::cout << std::endl << "Enter the server port (default: " << default_port << ")" << std::endl << std::endl;
        std::getline(std::cin, command_buffer);
        // Uses the default address if asked to.
        if(command_buffer.compare("default") == 0)
            server_port = default_port;
        else // Uses the provided port.
            server_port =  std::stoi(command_buffer);

        std::cout << std::endl << "Attempting connection to server (" << server_addr << ":" << server_port << ")..." << std::endl;

        // Creates the client and attempts to connect to the server.
        client clnt(server_addr.c_str(), server_port);

        // Checks for errors. 
        int cnct_status = clnt.get_status();
        if(cnct_status < 0) {
            std::cerr << COLOR_BOLD_RED << "Error connecting to remote socket! (" << cnct_status << ")" << COLOR_DEFAULT << std::endl << std::endl;
            return cnct_status;
        }
        std::cout << COLOR_BOLD_GREEN << "Connected to the server successfully!" << COLOR_DEFAULT << std::endl;

        // Handles the client execution until it's disconnected.
        clnt.handle();

        // When handle returns control the client will have disconencted from the server.
        std::cout << std::endl << COLOR_BOLD_RED << "Disconnected from the server!" << COLOR_DEFAULT << std::endl << std::endl;

        // Exits the program after disconencting.
        return 0;

    } 
    
    // Handles the server.
    if(inst_type == it_Server) {

        // Stores the port where the server will be hosted.
        int server_port;

        // Checks for the client parameters.
        if(argc == 2) // If no additional parameters were provided use the default ones.
            server_port = default_port;
        else if (argc == 3) // If a port is provided use it instead.
            server_port = std::stoi(std::string(argv[2]));
        else { // Displays help text if the number of parameters is invalid.
            std::cout << HELP_SERVER << std::endl;
            return 0;
        }
        
        std::cout << std::endl << "Creating server at port " << server_port << "..." << std::endl;

        // Creates the server on the given port.
        server srv(server_port);
        
        // Checks for errors. 
        int svr_status = srv.get_status();
        if(svr_status < 0) {
            std::cerr << COLOR_BOLD_RED << "Error connecting to socket! (" << svr_status << ")" << COLOR_DEFAULT << std::endl << std::endl;
            return svr_status;
        }
        std::cout << COLOR_BOLD_GREEN << "Server created successfully!" << COLOR_DEFAULT << std::endl << std::endl;

        // Handles the server execution.
        std::cout << "Running... " << COLOR_BOLD_CYAN << "<Press CTRL+C to stop>" << COLOR_DEFAULT << std::endl << std::endl;
        srv.handle();

        // When handle returns control the server will have closed.
        std::cout << COLOR_BOLD_YELLOW << std::endl << "Server closed!" << COLOR_DEFAULT << std::endl << std::endl;;

        // Exits the program after the server is closed.
        return 0;

    }

    // Exits the program, attempted to execute as an invalid instance type.
    return -1;

}
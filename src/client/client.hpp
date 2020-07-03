// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CLIENT_H
# define CLIENT_H

# include <string>
# include <vector>
# include <mutex>
# include <atomic>
# include <netinet/in.h>

class client
{

    public: 

        int network_socket;

        std::mutex updating_messages;
        std::vector<std::string> new_messages;

        // If admin commands should be shown.
        std::atomic_bool atmc_show_admin_commands;

        client();
        ~client();

        // Tries connecting to a server and returns the connection status.
        int connect_to_server(const char *s_addr, int port_number);

        // Handles the client instance (control of the program is given to the client until it disconnects).
        void handle();

        // Constantly listens to the server and does what's necessary.
        void t_listen_to_server();

        // Shows client new messages.
        void show_new_messages();

    private:

        struct sockaddr_in server_address;
        int connection_status;

};

# endif
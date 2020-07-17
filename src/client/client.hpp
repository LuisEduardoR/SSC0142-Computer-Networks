// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef CLIENT_H
# define CLIENT_H

# include <string>

# include <queue>

# include <mutex>
# include <atomic>

# include <netinet/in.h>

class client
{

    public: 

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        /* Creates a new client and tries connecting to a server. */
        client(const char *s_addr, int port_number);

        /* Closes the socket on the destructor. */
        ~client();

        // ==============================================================================================================================================================
        // Client =======================================================================================================================================================
        // ==============================================================================================================================================================

        /* Returns the status of the client */
        int get_status();

        // Handles the client instance (control of the program is given to the client until it disconnects).
        void handle();

        // Constantly listens to the server and does what's necessary.
        void t_listen_to_server();

        // Shows client new messages.
        void show_new_messages();

    private:

        // ==============================================================================================================================================================
        // Variables ====================================================================================================================================================
        // ==============================================================================================================================================================

        /* Used to store information about the client socket and address. */
        int network_socket;
        struct sockaddr_in server_address;

        /* Stores the status of the server */
        int client_status;

        std::mutex updating_messages;
        std::queue<std::string> new_messages;

        // If admin commands should be shown.
        std::atomic_bool atmc_show_admin_commands;

};

# endif
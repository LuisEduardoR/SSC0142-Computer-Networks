// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef REQUEST_H
# define REQUEST_H

# include <string>

// Used to identify the type of a request, i.e. what command it should execute.
enum request_type { rt_Invalid, rt_Send, rt_Nickname, rt_Join, rt_Admin_kick, rt_Admin_mute, rt_Admin_unmute, rt_Admin_whois};

class request
{

    public:

        // ==============================================================================================================================================================
        // Constructors/destructors =====================================================================================================================================
        // ==============================================================================================================================================================

        request();
        request(int origin_socket, request_type r_type, const std::string data);

        // ==============================================================================================================================================================
        // Getters ======================================================================================================================================================
        // ==============================================================================================================================================================

        // Getter for the origin socket.
        int get_origin_socket() const;

        // Getter for the request type.
        request_type get_type() const;

        // Getter for the data.
        std::string get_data() const;

    private:

        // ==============================================================================================================================================================
        // Variables ====================================================================================================================================================
        // ==============================================================================================================================================================

        /* The socket from which this request originated. */
        int origin_socket;

        /* Type of request. */
        request_type r_type;

        // Data received for the request.
        std::string data;

};

# endif
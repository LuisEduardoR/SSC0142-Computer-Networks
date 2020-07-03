// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# ifndef REQUEST_H
# define REQUEST_H

# include <string>

class request
{
    private:

        // The socket from which this request originated.
        int origin_socket;
        // Content of this request.
        std::string content;

    public:

        // CONSTRUCTOR
        request();
        request(int origin_socket, std::string content);

        // Getter for the origin socket.
        int get_origin_socket();
        // Getter for the content.
        std::string get_content();

};

# endif
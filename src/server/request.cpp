// Authors:
// Abner Eduardo Silveira Santos - NUSP 10692012
// João Pedro Uchôa Cavalcante - NUSP 10801169
// Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

# include "request.hpp"

# include <string>

// CONSTRUCTOR
request::request() {

    this->origin_socket = -1;
    this->content = "/none";

}

// CONSTRUCTOR
request::request(int origin_socket, std::string content) {

    this->origin_socket = origin_socket;
    this->content = content;

}

int request::get_origin_socket() { return this->origin_socket; }
std::string request::get_content() { return this->content; }
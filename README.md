This is an assignment made by:

* Abner Eduardo Silveira Santos - NUSP 10692012
* João Pedro Uchôa Cavalcante - NUSP 10801169
* Luís Eduardo Rozante de Freitas Pereira - NUSP 10734794

For the SSC0142 - Computer Networks course during the 1st semester of 2020 at ICMC - University of São Paulo.

The code was confirmed to work in a Fedora 32 Virtual Machine with the make and gcc packages updated to the newest stable version (2020-06-02).

The code can be compiled in a standard Linux system with the make and gcc packages by using the command:

    make all

The following lines assume the compiled file has not being renamed:

* The code can be run after compiled by using the command:

        ./trabalho-redes [parameters]

* To start a server with the default parameters run:

        ./trabalho-redes server

* To start a client with the default parameters tun:

        ./trabalho-redes client

* To see more detail about the parameters run:

        ./trabalho-redes --help

A test file is provided containing a /send command followed by more than 4096 characters and ending with a /quit command, this is intended to be redirected as input and used for tests.
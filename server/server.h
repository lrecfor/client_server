//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <sstream>


class Server {
public:
    static std::string list_files(std::string& path);

    static std::string list_processes(std::string directory);
};


class ServerHandler {
public:
    static void* handle_client(void* client_socket_ptr);

    static void run_server();
};


#endif //SERVER_H

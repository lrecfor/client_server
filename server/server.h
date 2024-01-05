//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <sstream>


#define PORT 8081
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10


class Server {

public:
    bool upload_files(std::string filename);
    std::vector<std::string> list_files(std::string directory);
    std::vector<std::string> list_processes(std::string directory);

};


class ServerHandler {
public:
    static void* handle_client(void* client_socket_ptr);
    static void run_server();
};



#endif //SERVER_H

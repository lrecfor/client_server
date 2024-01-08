//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <sstream>


#define MAX_CLIENTS 10
#define PATH "SERVER_/"


class Server {

public:
    // static bool send_text(const std::string& filename, int client_socket);
    // static bool receive_confirmation(int client_socket);
    // static ssize_t send_all(int socket, const void* buffer, size_t length);

    //static bool receive_file(const std::string& filename);

    //std::vector<std::string> list_files(std::string directory);
    static std::string list_files(std::string& path);
    std::vector<std::string> list_processes(std::string directory);

};


class ServerHandler {
public:
    static void* handle_client(void* client_socket_ptr);
    static void run_server();
};



#endif //SERVER_H

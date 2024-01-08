//
// Created by dana on 08.01.2024.
//

#ifndef UTILS_H
#define UTILS_H


#define SERVER_IP "127.0.0.1"
#define PORT 8081
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define PATH_S "SERVER_/"
#define PATH_C "CLIENT_/"


class Utiliter {
public:
    static std::string extractFilenameFromRequest(const char* request);

    static bool sendFile(const std::string& filename, int client_socket);

    static bool receiveFile(const std::string& filename, int client_socket);

private:
    static bool receiveConfirmation(int client_socket);

    static ssize_t sendAll(int socket, const void* buffer, size_t length);

    static std::string extractErrorMessage(const std::string& responseHeader);
};


#endif //UTILS_H

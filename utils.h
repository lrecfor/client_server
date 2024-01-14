//
// Created by dana on 08.01.2024.
//

#ifndef UTILS_H
#define UTILS_H

#include "ConfigHandler.h"


class Utiliter {

public:
    int BUFFER_SIZE;
    std::string PATH_S;
    std::string PATH_C;

    Utiliter() {
        PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
        BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");
        PATH_C = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_C");
    }

    static std::string extractFilenameFromRequest(const char* request);

    static std::string extractPidFromRequest(const char* request);

    bool sendFile(const std::string& filename, int client_socket);

    bool receiveFile(const std::string& filename, int client_socket);

    bool sendString(const std::string& string_, int client_socket);

    bool receiveString(int client_socket);

    static std::string extractErrorMessage(const std::string& responseHeader);

    static ssize_t sendAll(int socket, const void* buffer, size_t length);

private:
    static bool receiveConfirmation(int client_socket);
};


#endif //UTILS_H

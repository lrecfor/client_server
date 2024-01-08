//
// Created by dana on 08.01.2024.
//

#ifndef UTILS_H
#define UTILS_H


#define SERVER_IP "127.0.0.1"
#define PORT 8082
#define BUFFER_SIZE 1024


class Utiliter {
public:
    static bool send_text(const std::string& filename, int client_socket);

    static bool receive_and_save_file(const std::string& filename, int client_socket);

    static void send_error(int socket, const std::string& error_message);

    static bool receive_confirmation(int client_socket);

    static std::string extractErrorMessage(const std::string& responseHeader);

    static ssize_t send_all(int socket, const void* buffer, size_t length);
};



#endif //UTILS_H

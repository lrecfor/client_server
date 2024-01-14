//
// Created by dana on 05.01.2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <vector>

#include "../utils.h"
#include "../ConfigHandler.h"


class Client {
public:
    std::string SERVER_IP;
    int PORT;
    std::string PATH_C;

    Client() {
        SERVER_IP = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "connection", "SERVER_IP");
        PORT = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "PORT");
        PATH_C = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_C");
    }

    void runClient() const {
        sockaddr_in server_addr{};

        Utiliter ut;

        // Создаем сокет
        const int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("Error creating socket");
            exit(EXIT_FAILURE);
        }

        // Настраиваем параметры сервера
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP.c_str(), &(server_addr.sin_addr));

        // Устанавливаем соединение
        if (connect(client_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
            perror("Error connecting to server");
            exit(EXIT_FAILURE);
        }

        while (true) {
            std::string user_input;
            std::cout << "Enter a request (type 'exit' to quit): ";
            std::getline(std::cin, user_input);

            // Если введено "exit", выходим из цикла
            if (user_input == "exit") {
                break;
            }

            // Преобразуем введенную строку в const char*
            const char* request = user_input.c_str();

            // Отправляем запрос
            send(client_socket, request, strlen(request), 0);

            // Если отправлен запрос на загрузку файла, извлекаем имя файла и отправляем
            if (strncmp(request, "GET /download", 13) == 0) {
                if (std::string filename = Utiliter::extractFilenameFromRequest(request);
                    ut.receiveFile(PATH_C + filename, client_socket)) {
                    std::cout << "File received and saved as " << filename << std::endl;
                    } else {
                        std::cout << "Error downloading file " + filename << std::endl;
                    }
            } else if (strncmp(request, "POST /upload", 12) == 0) {
                if (std::string filename = Utiliter::extractFilenameFromRequest(request);
                    ut.sendFile(PATH_C + filename, client_socket)) {
                    std::cout << "File " + filename + " sent successfully" << std::endl;
                    } else {
                        std::cout << "Error uploading file " + filename << std::endl;
                        std::string error_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 20\r\n\r\n" +
                                                    std::string("Error uploading file");
                        send(client_socket, error_message.c_str(), strlen(error_message.c_str()), 0);
                    }
            } else {
                ut.receiveString(client_socket);
            }
        }

        // Закрываем сокет клиента
        close(client_socket);

        std::cout << "Exiting the client.\n";
    }
};

#endif // CLIENT_H

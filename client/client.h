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
    int BUFFER_SIZE;

    Client() {
        SERVER_IP = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "connection", "SERVER_IP");
        PORT = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "PORT");
        PATH_C = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_C");
        BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");
    }

    bool updateUndownloadedFiles(std::vector<std::string> files, int client_socket) const
    {
        Utiliter ut;

        // Отправляем клиенту информацию о количестве файлов для докачки
        std::ostringstream header;
        header << "HTTP/1.1 200 OK\r\nContent-Length: " << files.size() << "\r\n\r\n";

        // Отправляем заголовочник с количеством файлов для докачки
        if (send(client_socket, header.str().c_str(), header.str().length(), 0) == -1) {
            std::cerr << "Error sending header" << std::endl;
            return false;
        }

        // Получаем количество доставленных байт
        if (const int receivedBytes = Utiliter::receiveBytes(client_socket); receivedBytes == -1) {
            std::cerr << "Error receiving byts count" << std::endl;
            return false;
        }

        for (auto& filename: files) {
            // Отправляем заголовочник с именем файла
            std::ostringstream header2;
            header2 << "HTTP/1.1 200 OK\r\n" << "?filename=" << filename << " HTTP/1.1\r\n\r\n";

            if (send(client_socket, header2.str().c_str(), header2.str().length(), 0) == -1) {
                std::cerr << "Error sending header" << std::endl;
                return false;
            }

            // Получаем количество доставленных байт
            if (const int receivedBytes = Utiliter::receiveBytes(client_socket); receivedBytes == -1) {
                std::cerr << "Error receiving byts count" << std::endl;
                return false;
            }

            // Находим размер файла
            std::ifstream file;
            file.open(PATH_C + filename, std::ios::binary | std::ios::in);
            if (!file.is_open()) {
                std::cerr << "Error opening file: " << filename << std::endl;
                return false;
            }
            file.seekg(0, std::ios::end);
            ssize_t file_size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Отправляем серверу информацию о размере файла
            std::ostringstream header3;
            header3 << "HTTP/1.1 200 OK\r\nContent-Length: " << file_size << "\r\n\r\n";

            if (send(client_socket, header3.str().c_str(), header2.str().length(), 0) == -1) {
                std::cerr << "Error sending header" << std::endl;
                return false;
            }

            // Получаем количество доставленных байт
            if (const int receivedBytes = Utiliter::receiveBytes(client_socket); receivedBytes == -1) {
                std::cerr << "Error receiving byts count" << std::endl;
                return false;
            }

            // Получаем файл
            if (ut.receive_(PATH_C + filename.erase(filename.length() - 30, 30), client_socket, 1, file_size))
                std::cout << "File " << filename << " updated" << std::endl;
            else
                return false;
        }

        return true;
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

        if (std::vector<std::string> files = ut.checkForUndownloadedFiles(); !files.empty()) {
            std::string request = ("GET /update?count=" + std::to_string(files.size()));
            send(client_socket, request.c_str(), strlen(request.c_str()), 0);

            std::cout << "File update is in progress, please wait..." << std::endl;
            if (updateUndownloadedFiles(files, client_socket))
                std::cout << "File update completed successfully" << std::endl;
            else
                std::cout << "File update failed" << std::endl;
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
                    ut.receive_(PATH_C + filename, client_socket, 1, 0)) {
                    std::cout << "File received and saved as " << filename << std::endl;
                } else {
                    std::cout << "Error downloading file " + filename << std::endl;
                }
            } else if (strncmp(request, "POST /upload", 12) == 0) {
                if (std::string filename = Utiliter::extractFilenameFromRequest(request);
                    ut.send_(PATH_C + filename, client_socket, 1, 0)) {
                    std::cout << "File " + filename + " sent successfully" << std::endl;
                } else {
                    std::cout << "Error uploading file " + filename << std::endl;
                    std::string error_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 20\r\n\r\n" +
                                                std::string("Error uploading file");
                    send(client_socket, error_message.c_str(), strlen(error_message.c_str()), 0);
                }
            } else {
                ut.receive_("", client_socket, 2, 0);
            }
        }

        // Закрываем сокет клиента
        close(client_socket);

        std::cout << "Exiting the client.\n";
    }
};

#endif // CLIENT_H

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
#include <algorithm>

#include "../utils.h"


#define PATH "CLIENT_/"


class Client {
public:
    static void run_client() {
        sockaddr_in server_addr{};

        // Создаем сокет
        const int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("Error creating socket");
            exit(EXIT_FAILURE);
        }

        // Настраиваем параметры сервера
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr));

        // Устанавливаем соединение
        if (connect(client_socket, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
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
            const char *request = user_input.c_str();

            // Отправляем запрос
            send(client_socket, request, strlen(request), 0);

            // Если отправлен запрос на загрузку файла, извлекаем имя файла и отправляем
            if (strncmp(request, "GET /download", 13) == 0) {
                std::string filename = extract_filename_from_request(request);
                Utiliter::receive_and_save_file(std::string(PATH) + filename, client_socket);
            } else if (strncmp(request, "POST /upload", 12) == 0) {
                std::string filename = extract_filename_from_request(request);

                if (!filename.empty()) {
                    if (Utiliter::send_text(std::string(PATH) + filename, client_socket)) {
                        std::cout << "File uploaded successfully.\n";
                    } else {
                        std::cerr << "Error uploading file.\n";
                    }
                } else {
                    std::cerr << "Invalid upload request.\n";
                }
            } else {
                // Получение ответа от сервера (может потребоваться в зависимости от вашей логики)
                char buffer[BUFFER_SIZE];
                if (const ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0); bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    std::cout << "Server response: " << buffer << std::endl;
                }
            }
        }

        // Закрываем сокет клиента
        close(client_socket);

        std::cout << "Exiting the client.\n";
    }

private:
    static std::string extract_filename_from_request(const char* request) {
        /*
         * Извлекает имя файла из заголовочника.
         */
        std::string request_str(request);
        if (size_t filename_start = request_str.find("filename="); filename_start != std::string::npos) {
            filename_start += 9;  // Перемещаем указатель после "filename="
            if (const size_t filename_end = request_str.find(' ', filename_start); filename_end != std::string::npos) {
                return request_str.substr(filename_start, filename_end - filename_start);
            }
        }
        return "";
    }

};

#endif // CLIENT_H

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


#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
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
                const char* filename = extract_filename_from_request(request);
                receive_and_save_file(filename, client_socket);
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
    static const char* extract_filename_from_request(const char* request) {
        if (const char* filename_start = strstr(request, "filename=")) {
            filename_start += 9;  // Перемещаем указатель после "filename="
            if (const char* filename_end = strchr(filename_start, ' ')) {
                return std::string(filename_start, filename_end - filename_start).c_str();
            }
        }
        return nullptr;
    }

    static void receive_and_save_file(const char* filename, int client_socket) {
        std::ofstream received_file(std::string(PATH) + filename, std::ios::binary);
        if (!received_file.is_open()) {
            std::cerr << "Error creating " << filename << std::endl;
            return;
        }

        // Чтение заголовка с информацией о размере файла
        char header[BUFFER_SIZE];
        ssize_t header_received = recv(client_socket, header, BUFFER_SIZE, 0);
        if (header_received <= 0) {
            std::cerr << "Error receiving file header" << std::endl;
            received_file.close();
            return;
        }

        // Парсинг заголовка для получения размера файла
        std::string header_str(header, header_received);
        size_t content_length_pos = header_str.find("Content-Length:");
        if (content_length_pos == std::string::npos) {
            std::cerr << "Invalid file header" << std::endl;
            received_file.close();
            return;
        }

        size_t content_length_end = header_str.find("\r\n", content_length_pos);
        if (content_length_end == std::string::npos) {
            std::cerr << "Invalid file header" << std::endl;
            received_file.close();
            return;
        }

        std::string content_length_str = header_str.substr(content_length_pos + 15, content_length_end - (content_length_pos + 15));
        size_t file_size = std::stoul(content_length_str);

        // Чтение данных файла и запись в файл
        ssize_t bytes_received;
        size_t total_bytes_received = 0;

        while (total_bytes_received < file_size) {
            char buffer[BUFFER_SIZE];
            bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                std::cerr << "Error receiving file content" << std::endl;
                received_file.close();
                return;
            }

            received_file.write(buffer, bytes_received);
            total_bytes_received += bytes_received;
        }

        received_file.close();
        std::cout << "File received and saved as " << filename << std::endl;
    }



};

#endif // CLIENT_H

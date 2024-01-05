#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sstream>

#include "server.h"

#define PORT 8082
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10


void handle_client(const int client_socket) {
    auto server = Server();

    char buffer[BUFFER_SIZE];

    while (true) {
        // Принимаем запрос от клиента
        const ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Если recv возвращает 0 или отрицательное значение, клиент отключился
            if (bytes_received == 0) {
                std::cout << "Client disconnected.\n";
            } else {
                perror("Error receiving data");
            }

            break; // Выходим из цикла обработки клиента
        }

        buffer[bytes_received] = '\0';  // Null-terminate the received data

        // Обработка запроса в зависимости от протокола
        std::string response_message;

        if (strncmp(buffer, "POST /upload", 12) == 0) {
            // Логика обработки загрузки файла
            std::cout << buffer << "\n";
            std::cout << "Handling file upload...\n";
            if (server.upload_files("filename")) {
                response_message = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nUpload OK";
            } else {
                response_message = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 23\r\n\r\nUpload failed";
            }
        } else if (strncmp(buffer, "GET /list_files", 15) == 0) {
            // Логика обработки запроса листинга файлов
            std::cout << "Handling list files request...\n";
            std::vector<std::string> files_list = server.list_files("local");

            // Преобразование вектора в строку
            std::ostringstream ss;
            for (const auto &file : files_list) {
                ss << file << "\n";
            }

            response_message = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(ss.str().length()) + "\r\n\r\n" + ss.str();
        } else if (strncmp(buffer, "GET /list_processes", 19) == 0) {
            // Логика обработки запроса списка процессов
            std::cout << "Handling list processes request...\n";
            std::vector<std::string> processes_list = server.list_processes("ps");

            // Преобразование вектора в строку
            std::ostringstream ss;
            for (const auto &process : processes_list) {
                ss << process << "\n";
            }

            response_message = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(ss.str().length()) + "\r\n\r\n" + ss.str();
        } else {
            // Нераспознанный запрос
            response_message = "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nInvalid request.";
        }

        // Отправляем ответ клиенту
        send(client_socket, response_message.c_str(), response_message.length(), 0);
    }

    // Закрываем сокет клиента
    close(client_socket);
}


int main() {
    sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Создаем сокет
    const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Настраиваем параметры сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Привязываем сокет к адресу и порту
    if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Слушаем порт
    if (listen(server_socket, 10) == -1) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Создаем массив структур pollfd
    struct pollfd fds[MAX_CLIENTS + 1]; // +1 для слушающего сокета

    // Инициализируем слушающий сокет
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;

    // Инициализируем массив клиентских сокетов
    for (int i = 1; i < MAX_CLIENTS + 1; ++i) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    while (true) {
        // Ждем событий на сокетах
        if (poll(fds, MAX_CLIENTS + 1, -1) == -1) {
            perror("Error in poll");
            exit(EXIT_FAILURE);
        }

        // Обработка событий
        for (int i = 1; i < MAX_CLIENTS + 1; ++i) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == -1) {
                    // Пропускаем закрытый сокет
                    continue;
                }

                // Обработка события на клиентском сокете
                handle_client(fds[i].fd);
            }
        }

        // Проверка слушающего сокета
        if (fds[0].revents & POLLIN) {
            // Принимаем нового клиента
            const int client_socket = accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_addr), &addr_len);
            if (client_socket == -1) {
                perror("Error accepting connection");
                continue;
            }

            std::cout << "Connection accepted from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "\n";

            // Ищем свободное место в массиве клиентских сокетов
            int j;
            for (j = 1; j < MAX_CLIENTS + 1; ++j) {
                if (fds[j].fd == -1) {
                    fds[j].fd = client_socket;
                    fds[j].events = POLLIN;
                    break;
                }
            }

            // Если нет свободного места, закрываем сокет
            if (j == MAX_CLIENTS + 1) {
                close(client_socket);
            }
        }
    }


    // Закрываем серверный сокет
    close(server_socket);

    return 0;
}
//
// Created by dana on 05.01.2024.
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8082
#define BUFFER_SIZE 1024

int main() {
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

        // Пример: Получение ответа от сервера (может потребоваться в зависимости от вашей логики)
        char buffer[BUFFER_SIZE];
        if (const ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0); bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
        }
    }

    // Закрываем сокет клиента
    close(client_socket);

    std::cout << "Exiting the client.\n";
    return 0;
}

    // Примеры запросов
    // const auto upload_request = "POST /upload HTTP/1.1\r\nHost: server_ip:port\r\nContent-Length: file_size\r\n\r\n[Binary file data]";
    // const auto list_files_request = "GET /list_files HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    // const auto list_processes_request = "GET /list_processes HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    //
    // // Отправка запросов
    // std::cout << "Sending upload request...\n";
    // Client::send_request(upload_request);
    //
    // std::cout << "Sending list files request...\n";
    // Client::send_request(list_files_request);
    //
    // std::cout << "Sending list processes request...\n";
    // Client::send_request(list_processes_request);
    //
    // return 0;

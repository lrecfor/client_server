//
// Created by dana on 05.01.2024.
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <pthread.h>
#include <fstream>
#include <unistd.h>

#include "server.h"


bool Server::send_text(const std::string& filename, int client_socket) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    // Находим размер файла
    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\nContent-Length: " << file_size << "\r\n\r\n";

    // Отправляем заголовочник с размером файла
    if (send_all(client_socket, header.str().c_str(), header.str().length()) == -1) {
        std::cerr << "Error sending header to client" << std::endl;
        file.close();
        return false;
    }

    // Получаем подтверждение о доставке пакета
    if (!receive_confirmation(client_socket)) {
        std::cerr << "Error receiving confirmation from client" << std::endl;
        file.close();
        return false;
    }

    while (!file.eof()) {
        constexpr std::streamsize buffer_size = BUFFER_SIZE;
        char buffer[buffer_size];
        file.read(buffer, buffer_size);

        if (send_all(client_socket, buffer, file.gcount()) == -1) {
            std::cerr << "Error sending file content to client" << std::endl;
            file.close();
            return false;
        }

        // Ожидание подтверждения от клиента
        if (!receive_confirmation(client_socket)) {
            std::cerr << "Error receiving confirmation from client" << std::endl;
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool Server::receive_confirmation(const int client_socket) {
    char confirmation_message[10];
    const ssize_t bytes_received = recv(client_socket, confirmation_message, sizeof(confirmation_message), 0);

    if (bytes_received <= 0) {
        return false;  // Ошибка при получении подтверждения
    }

    confirmation_message[bytes_received] = '\0';

    // Проверка подтверждения от клиента
    if (std::string(confirmation_message) != "ACK") {
        return false;  // Некорректное подтверждение
    }

    return true;
}

// Функция для отправки всех данных из буфера
ssize_t Server::send_all(const int socket, const void* buffer, const size_t length) {
    const auto buffer_ptr = static_cast<const char*>(buffer);
    ssize_t total_sent = 0;

    while (total_sent < length) {
        const ssize_t sent = send(socket, buffer_ptr + total_sent, length - total_sent, 0);
        if (sent == -1) {
            return -1;  // Ошибка отправки
        }

        total_sent += sent;
    }

    std::cout << "Total sent: " << total_sent << std::endl;

    return total_sent;
}

std::vector<std::string> Server::list_files(std::string directory) {
    return std::vector<std::string>({"first, second"});
}

std::vector<std::string> Server::list_processes(std::string directory) {
    return std::vector<std::string>({"third, fourth"});
}

void* ServerHandler::handle_client(void* client_socket_ptr) {
    int client_socket = *static_cast<int *>(client_socket_ptr);
    delete static_cast<int *>(client_socket_ptr);

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

        if (strncmp(buffer, "GET /download", 13) == 0) {
            std::cout << "Handling file download...\n";

            // Получаем имя файла из запроса (пример: GET /download?filename=myfile.txt)
            std::string filename;
            if (size_t pos = std::string(buffer).find("filename="); pos != std::string::npos) {
                // Найден "filename=", теперь нужно найти конец имени файла (первый пробел или конец строки)
                if (size_t end_pos = std::string(buffer).find_first_of(" \r\n", pos + 9); end_pos != std::string::npos) {
                    filename = std::string(buffer).substr(pos + 9, end_pos - pos - 9);
                } else {
                    // Если конец строки не найден, считаем, что имя файла занимает оставшуюся часть строки
                    filename = std::string(buffer).substr(pos + 9);
                }
            }

            if (!filename.empty()) {
                if (server.send_text(PATH + filename, client_socket)) {
                    std::cout << "File downloaded successfully.\n";
                } else {
                    std::cerr << "Error downloading file.\n";
                }
            } else {
                std::cerr << "Invalid download request.\n";
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

    pthread_exit(nullptr);
}

void ServerHandler::run_server() {
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
        for (int i = 0; i < MAX_CLIENTS + 1; ++i) {
            // Проверяем слушающий сокет
            if (i == 0 && fds[i].revents & POLLIN) {
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
                        break;
                    }
                }

                // Если нет свободного места, закрываем сокет
                if (j == MAX_CLIENTS + 1) {
                    close(client_socket);
                }

                pthread_t client_thread;
                int *client_socket_ptr = new int(client_socket);

                if (pthread_create(&client_thread, nullptr, handle_client, (void*)client_socket_ptr) != 0) {
                    perror("Error creating thread");
                    exit(EXIT_FAILURE);
                }

                // Позволяет освободить ресурсы потока после завершения
                pthread_detach(client_thread);
            } else if (fds[i].revents & POLLIN) {
                // Обработка события на клиентском сокете
                // В данной реализации клиентские сокеты обрабатываются в отдельных потоках, поэтому в основном цикле мы не делаем ничего.
            }
        }
    }

    // Закрываем серверный сокет
    close(server_socket);
}

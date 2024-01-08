//
// Created by dana on 08.01.2024.
//


#include <iostream>
#include <sys/socket.h>
#include <fstream>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
#include <fstream>
#include <ctime>
#include <regex>
#include <string>


#include "utils.h"


bool Utiliter::send_text(const std::string& filename, int client_socket) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        send_error(client_socket, "Error opening file");
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
        std::cerr << "Error sending header" << std::endl;
        send_error(client_socket, "Error sending header");
        file.close();
        return false;
    }

    // Получаем подтверждение о доставке пакета
    if (!receive_confirmation(client_socket)) {
        std::cerr << "Error receiving confirmation" << std::endl;
        send_error(client_socket, "Error receiving confirmation");
        file.close();
        return false;
    }

    while (!file.eof()) {
        constexpr std::streamsize buffer_size = BUFFER_SIZE;
        char buffer[buffer_size];
        file.read(buffer, buffer_size);

        if (send_all(client_socket, buffer, file.gcount()) == -1) {
            std::cerr << "Error sending file content" << std::endl;
            send_error(client_socket, "Error sending file content");
            file.close();
            return false;
        }

        // Ожидание подтверждения от второй стороны
        if (!receive_confirmation(client_socket)) {
            std::cerr << "Error receiving confirmation" << std::endl;
            send_error(client_socket, "Error receiving confirmation");
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool Utiliter::receive_and_save_file(const std::string& filename, int client_socket) {
    // Получаем заголовочник с размером файла
    char header[BUFFER_SIZE];
    ssize_t header_received = recv(client_socket, header, BUFFER_SIZE, 0);

    if (header_received <= 0) {
        std::cerr << "Error receiving file header" << std::endl;
        send_error(client_socket, "Error receiving file header");
        return false;
    }

    // Проверка на получение сообщения о возникновении ошибки
    std::string header_str(header, header_received);
    if (strncmp(header_str.c_str(), "HTTP/1.1 500 Internal Server Error", 34) == 0 ) {
        std::string error_message = extractErrorMessage(header_str);
        std::cerr << "Server error message: " << error_message << std::endl;
        return false;
    }

    // Отправляем подтверждение если ошибок не возникло
    send(client_socket, "ACK", 3, 0);

    // Создаем файл
    std::ofstream received_file(filename, std::ios::binary);
    if (!received_file.is_open()) {
        std::cerr << "Error creating file" << filename << std::endl;
        send_error(client_socket, "Error creating file");
        return false;
    }

    // Парсинг заголовочника, получаем размер файла
    //std::string header_str(header, header_received);
    size_t content_length_pos = header_str.find("Content-Length:");

    if (content_length_pos == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        send_error(client_socket, "Invalid file header");
        received_file.close();
        return false;
    }

    size_t content_length_end = header_str.find("\r\n", content_length_pos);

    if (content_length_end == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        send_error(client_socket, "Invalid file header");
        received_file.close();
        return false;
    }

    std::string content_length_str = header_str.substr(content_length_pos + 15, content_length_end - (content_length_pos + 15));
    size_t file_size = std::stoul(content_length_str); // Размер файла

    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    // Цикл для загрузки файла
    while (total_bytes_received < file_size) {
        char buffer[BUFFER_SIZE];
        bytes_received = recv(client_socket, buffer, std::min<size_t>(BUFFER_SIZE, file_size - total_bytes_received), 0);

        if (bytes_received <= 0) {
            std::cerr << "Error receiving file content" << std::endl;
            send_error(client_socket, "Error receiving file content");
            received_file.close();
            return false;
        }

        received_file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;

        // Отправка подтверждения
        send(client_socket, "ACK", 3, 0);
    }

    received_file.close();
    std::cout << "File received and saved as " << filename << std::endl;
    return true;
}

// Функция для отправки сообщения об ошибке
void Utiliter::send_error(int socket, const std::string& error_message) {
    std::ostringstream error_response;
    error_response << "HTTP/1.1 500 Internal Server Error\r\nContent-Length: " << error_message.length() << "\r\n\r\n" << error_message;
    send_all(socket, error_response.str().c_str(), error_response.str().length());
}

bool Utiliter::receive_confirmation(const int client_socket) {
    char confirmation_message[10];
    const ssize_t bytes_received = recv(client_socket, confirmation_message, sizeof(confirmation_message), 0);

    if (bytes_received <= 0) {
        return false;  // Ошибка при получении подтверждения
    }

    confirmation_message[bytes_received] = '\0';

    // Проверка подтверждения от другой стороны
    if (std::string(confirmation_message) != "ACK") {
        return false;  // Некорректное подтверждение
    }

    return true;
}

std::string Utiliter::extractErrorMessage(const std::string& responseHeader) {
    size_t errorPosition = responseHeader.find("\r\n\r\n");

    if (errorPosition != std::string::npos && errorPosition + 4 < responseHeader.length()) {
        // Извлекаем текст ошибки
        return responseHeader.substr(errorPosition + 4);
    }

    return "Ошибка не найдена.";
}

// Функция для отправки всех данных из буфера
ssize_t Utiliter::send_all(const int socket, const void* buffer, const size_t length) {
    const auto buffer_ptr = static_cast<const char*>(buffer);
    ssize_t total_sent = 0;

    while (total_sent < length) {
        const ssize_t sent = send(socket, buffer_ptr + total_sent, length - total_sent, 0);
        if (sent == -1) {
            return -1;  // Ошибка отправки
        }

        total_sent += sent;
    }

    return total_sent;
}
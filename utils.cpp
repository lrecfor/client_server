//
// Created by dana on 08.01.2024.
//


#include <iostream>
#include <sys/socket.h>
#include <fstream>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <ctime>
#include <regex>
#include <string>


#include "utils.h"


std::string Utiliter::extractFilenameFromRequest(const char* request) {
    /*
     * Получаем имя файла из запроса (пример: GET /download?filename=myfile.txt).
     */
    std::string filename;
    if (const size_t pos = std::string(request).find("filename="); pos != std::string::npos) {
        // Найден "filename=", теперь нужно найти конец имени файла (первый пробел или конец строки)
        if (const size_t end_pos = std::string(request).find_first_of(" \r\n", pos + 9); end_pos != std::string::npos) {
            return std::string(request).substr(pos + 9, end_pos - pos - 9);
        }
        // Если конец строки не найден, считаем, что имя файла занимает оставшуюся часть строки
        return std::string(request).substr(pos + 9);
    }
    return "";
}

std::string Utiliter::extractPidFromRequest(const char* request) {
    /*
     * Получаем имя файла из запроса (пример: GET /process_info?pid=2).
     */
    std::string pid;
    if (const size_t pos = std::string(request).find("pid="); pos != std::string::npos) {
        // Найден "pid=", теперь нужно найти конец номера процесса (первый пробел или конец строки)
        if (const size_t end_pos = std::string(request).find_first_of(" \r\n", pos + 4); end_pos != std::string::npos) {
            return std::string(request).substr(pos + 4, end_pos - pos - 4);
        }
        // Если конец строки не найден, считаем, что номер процесса занимает оставшуюся часть строки
        return std::string(request).substr(pos + 4);
    }
    return "";
}

bool Utiliter::sendFile(const std::string& filename, int client_socket) {
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
    if (sendAll(client_socket, header.str().c_str(), header.str().length()) == -1) {
        std::cerr << "Error sending header" << std::endl;
        file.close();
        return false;
    }

    // Получаем подтверждение о доставке пакета
    if (!receiveConfirmation(client_socket)) {
        std::cerr << "Error receiving confirmation" << std::endl;
        file.close();
        return false;
    }

    if (file_size == 0) {
        std::cout << "File is empty" << std::endl;
        file.close();
        return true;
    }

    while (!file.eof()) {
        char buffer[BUFFER_SIZE];
        file.read(buffer, BUFFER_SIZE);

        if (sendAll(client_socket, buffer, file.gcount()) == -1) {
            std::cerr << "Error sending file content" << std::endl;
            file.close();
            return false;
        }

        // Ожидание подтверждения от второй стороны
        if (!receiveConfirmation(client_socket)) { //tckb gjlndth;ltybt yt gjkextyj. nj ljrfxfnm afqk fdnjvfnbxtcrb
            std::cerr << "Error receiving confirmation" << std::endl;
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool Utiliter::receiveFile(const std::string& filename, int client_socket) {
    // Получаем заголовочник с размером файла
    char header[BUFFER_SIZE];
    ssize_t header_received = recv(client_socket, header, BUFFER_SIZE, 0);

    if (header_received <= 0) {
        std::cerr << "Error receiving file header" << std::endl;
        return false;
    }

    // Проверка на получение сообщения о возникновении ошибки
    std::string header_str(header, header_received);
    if (strncmp(header_str.c_str(), "HTTP/1.1 500 Internal Error", 27) == 0) {
        std::string error_message = extractErrorMessage(header_str);
        std::cerr << error_message << std::endl;
        return false;
    }

    // Отправляем подтверждение если ошибок не возникло
    send(client_socket, "ACK", 3, 0);

    // Создаем файл
    std::ofstream received_file(filename, std::ios::binary);
    if (!received_file.is_open()) {
        std::cerr << "Error creating file" << filename << std::endl;
        return false;
    }

    // Парсинг заголовочника, получаем размер файла
    //std::string header_str(header, header_received);
    size_t content_length_pos = header_str.find("Content-Length:");

    if (content_length_pos == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        received_file.close();
        return false;
    }

    size_t content_length_end = header_str.find("\r\n", content_length_pos);

    if (content_length_end == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        received_file.close();
        return false;
    }

    std::string content_length_str = header_str.substr(content_length_pos + 15,
                                                       content_length_end - (content_length_pos + 15));
    size_t file_size = std::stoul(content_length_str); // Размер файла

    timeval timeout{};
    timeout.tv_sec = 3;  // секунды
    timeout.tv_usec = 0;  // микросекунды

    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error setting timeout" << std::endl;
        return false;
    }

    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    // Цикл для загрузки файла
    while (total_bytes_received < file_size) {
        char buffer[BUFFER_SIZE];
        bytes_received = recv(client_socket, buffer, std::min<size_t>(BUFFER_SIZE, file_size - total_bytes_received),
                              0); //recv jnghfdkztn yt dct ,fqns vj;tn nj j;blfnm dct 1024 b gjnjv jnghfdkznm gjlndth;ltybt j ghbyznbb 1024 ,fqnjd

        if (bytes_received <= 0) {
            std::cerr << "Error receiving file content" << std::endl;
            received_file.close();
            return false;
        }

        received_file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;

        // Отправка подтверждения
        send(client_socket, "ACK", 3, 0);
    }

    received_file.close();
    return true;
}

bool Utiliter::sendString(const std::string& string_, int client_socket) {
    // Находим размер строки
    const int strSize = string_.length();

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\nContent-Length: " << strSize << "\r\n\r\n";

    // Отправляем заголовочник с размером файла
    if (sendAll(client_socket, header.str().c_str(), header.str().length()) == -1) {
        std::cerr << "Error sending header" << std::endl;
        return false;
    }

    // Получаем подтверждение о доставке пакета
    if (!receiveConfirmation(client_socket)) {
        std::cerr << "Error receiving confirmation" << std::endl;
        return false;
    }

    if (strSize == 0) {
        std::cout << "File is empty" << std::endl;
        return true;
    }

    size_t totalBytesSent = 0;

    while (totalBytesSent < string_.length()) {
        const size_t bytesToSend = std::min(static_cast<size_t>(BUFFER_SIZE), strSize - totalBytesSent);
        const ssize_t sentBytes = send(client_socket, string_.c_str() + totalBytesSent, bytesToSend, 0);

        if (sentBytes == -1) {
            std::cerr << "Error sending data" << std::endl;
            return false;
        }

        // Ожидание подтверждения от второй стороны
        if (!receiveConfirmation(client_socket)) {
            std::cerr << "Error receiving confirmation" << std::endl;
            return false;
        }

        totalBytesSent += static_cast<size_t>(sentBytes);
    }

    return true;
}

bool Utiliter::receiveString(int client_socket) {
    // Получаем заголовочник с размером файла
    char header[BUFFER_SIZE];
    ssize_t header_received = recv(client_socket, header, BUFFER_SIZE, 0);

    if (header_received <= 0) {
        std::cerr << "Error receiving file header" << std::endl;
        return false;
    }

    // Проверка на получение сообщения о возникновении ошибки
    std::string header_str(header, header_received);
    if (strncmp(header_str.c_str(), "HTTP/1.1 500 Internal Error", 27) == 0) {
        std::string error_message = extractErrorMessage(header_str);
        std::cerr << error_message << std::endl;
        return false;
    }

    // Отправляем подтверждение если ошибок не возникло
    send(client_socket, "ACK", 3, 0);

    // Парсинг заголовочника, получаем размер строки
    //std::string header_str(header, header_received);
    size_t content_length_pos = header_str.find("Content-Length:");

    if (content_length_pos == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        return false;
    }

    size_t content_length_end = header_str.find("\r\n", content_length_pos);

    if (content_length_end == std::string::npos) {
        std::cerr << "Invalid file header" << std::endl;
        return false;
    }

    std::string content_length_str = header_str.substr(content_length_pos + 15,
                                                       content_length_end - (content_length_pos + 15));
    size_t strSize = std::stoul(content_length_str); // Размер строки

    timeval timeout{};
    timeout.tv_sec = 3;  // секунды
    timeout.tv_usec = 0;  // микросекунды

    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error setting timeout" << std::endl;
        return false;
    }

    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    // Цикл для вывода строки
    while (total_bytes_received < strSize) {
        char buffer[BUFFER_SIZE];
        bytes_received = recv(client_socket, buffer, std::min<size_t>(BUFFER_SIZE, strSize - total_bytes_received),
                              0);

        if (bytes_received <= 0) {
            std::cerr << "\nError receiving file content" << std::endl;
            return false;
        }

        std::cout.write(buffer, bytes_received);
        total_bytes_received += bytes_received;

        // Отправка подтверждения
        send(client_socket, "ACK", 3, 0);
    }

    return true;
}

bool Utiliter::receiveConfirmation(const int client_socket) {
    /*
     * Функция для проверки получения подстверждения.
     */

    char confirmation_message[10];
    const ssize_t bytes_received = recv(client_socket, confirmation_message, sizeof(confirmation_message), 0);

    if (bytes_received <= 0) {
        return false; // Ошибка при получении подтверждения
    }

    confirmation_message[bytes_received] = '\0';

    if (std::string(confirmation_message) != "ACK") {
        return false; // Некорректное подтверждение
    }

    return true;
}

std::string Utiliter::extractErrorMessage(const std::string& responseHeader) {
    /*
     * Функция для извлечения текста ошибки из запроса.
     */

    if (const size_t errorPosition = responseHeader.find("\r\n\r\n");
        errorPosition != std::string::npos && errorPosition + 4 < responseHeader.length()) {
        // Извлекаем текст ошибки
        return responseHeader.substr(errorPosition + 4);
    }

    return "Ошибка не найдена.";
}

ssize_t Utiliter::sendAll(const int socket, const void* buffer, const size_t length) {
    /*
     * Функция для отправки всех данных из буфера.
     */
    const auto buffer_ptr = static_cast<const char *>(buffer);
    ssize_t total_sent = 0;

    while (total_sent < length) {
        const ssize_t sent = send(socket, buffer_ptr + total_sent, length - total_sent, 0);
        if (sent == -1) {
            return -1; // Ошибка отправки
        }

        total_sent += sent;
    }

    return total_sent;
}

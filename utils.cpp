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

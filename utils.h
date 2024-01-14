//
// Created by dana on 08.01.2024.
//

#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
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


#include "ConfigHandler.h"


class Utiliter {
public:
    int BUFFER_SIZE;
    std::string PATH_S;
    std::string PATH_C;

    Utiliter() {
        PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
        BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");
        PATH_C = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_C");
    }

    static std::string extractFilenameFromRequest(const char* request) {
        /*
         * Получаем имя файла из запроса (пример: GET /download?filename=myfile.txt).
         */
        std::string filename;
        if (const size_t pos = std::string(request).find("filename="); pos != std::string::npos) {
            // Найден "filename=", теперь нужно найти конец имени файла (первый пробел или конец строки)
            if (const size_t end_pos = std::string(request).find_first_of(" \r\n", pos + 9);
                end_pos != std::string::npos) {
                return std::string(request).substr(pos + 9, end_pos - pos - 9);
            }
            // Если конец строки не найден, считаем, что имя файла занимает оставшуюся часть строки
            return std::string(request).substr(pos + 9);
        }
        return "";
    }

    static std::string extractPidFromRequest(const char* request) {
        /*
         * Получаем имя файла из запроса (пример: GET /process_info?pid=2).
         */
        std::string pid;
        if (const size_t pos = std::string(request).find("pid="); pos != std::string::npos) {
            // Найден "pid=", теперь нужно найти конец номера процесса (первый пробел или конец строки)
            if (const size_t end_pos = std::string(request).find_first_of(" \r\n", pos + 4);
                end_pos != std::string::npos) {
                return std::string(request).substr(pos + 4, end_pos - pos - 4);
            }
            // Если конец строки не найден, считаем, что номер процесса занимает оставшуюся часть строки
            return std::string(request).substr(pos + 4);
        }
        return "";
    }

    static std::string extractErrorMessage(const std::string& responseHeader) {
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

    [[nodiscard]] bool send_(const std::string& data, int client_socket, int mode) const {
        //Находим размер файла
        size_t data_size = 0;
        std::ifstream file;

        if (mode == 1) {
            file.open(data, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Error opening file: " << data << std::endl;
                return false;
            }
            file.seekg(0, std::ios::end);
            data_size = file.tellg();
            file.seekg(0, std::ios::beg);
        } else if (mode == 2) {
            data_size = data.length();
        } else {
            std::cerr << "Error mode" << std::endl;
        }

        std::ostringstream header;
        header << "HTTP/1.1 200 OK\r\nContent-Length: " << data_size << "\r\n\r\n";

        // Отправляем заголовочник с размером файла
        if (send(client_socket, header.str().c_str(), header.str().length(), 0) == -1) {
            std::cerr << "Error sending header" << std::endl;
            file.close();
            return false;
        }

        // Получаем количество доставленных байт
        int receivedBytes = receiveBytes(client_socket);
        if (receivedBytes == -1) {
            std::cerr << "Error receiving byts count" << std::endl;
            return false;
        }

        if (data_size == 0) {
            std::cout << "File is empty" << std::endl;
            if (mode == 1)
                file.close();
            return true;
        }

        if (mode == 1) {
            size_t totalBytesSent = 0;

            while (!file.eof() && totalBytesSent < data_size) {
                char buffer[BUFFER_SIZE];
                file.read(buffer, BUFFER_SIZE);

                if (send(client_socket, buffer, file.gcount(), 0) == -1) {
                    std::cerr << "Error sending file content" << std::endl;
                    file.close();
                    return false;
                }

                // Ожидание количества полученных байт
                int receivedBytes = receiveBytes(client_socket);
                if (receivedBytes == -1) {
                    std::cerr << "Error receiving byts count" << std::endl;
                    return false;
                }

                totalBytesSent += static_cast<size_t>(receivedBytes);
                file.seekg(totalBytesSent, std::ios::beg);
            }

            file.close();
        } else {
            size_t totalBytesSent = 0;

            while (totalBytesSent < data_size) {
                const size_t bytesToSend = std::min(static_cast<size_t>(BUFFER_SIZE), data_size - totalBytesSent);
                const ssize_t sentBytes = send(client_socket, data.c_str() + totalBytesSent, bytesToSend, 0);

                if (sentBytes == -1) {
                    std::cerr << "Error sending data" << std::endl;
                    return false;
                }

                // Ожидание количества полученных байт
                int receivedBytes = receiveBytes(client_socket);
                if (receivedBytes == -1) {
                    std::cerr << "Error receiving byts count" << std::endl;
                    return false;
                }

                totalBytesSent += static_cast<size_t>(receivedBytes);
            }
        }
        return true;
    }

    [[nodiscard]] bool receive_(const std::string& filename, int client_socket, int mode) const {
        std::ofstream received_file;

        // Получаем заголовочник с размером файла
        char header[BUFFER_SIZE];
        ssize_t header_received = recv(client_socket, header, BUFFER_SIZE, 0);

        if (header_received <= 0) {
            std::cerr << "Error receiving file header" << std::endl;
            return false;
        }

        // Проверка на получение сообщения о возникновении ошибки
        std::string header_str(header, header_received);
        if (strncmp(header_str.c_str(), "HTTP/1.1 500 Internal Error", 27) == 0 ||
            strncmp(header_str.c_str(), "HTTP/1.1 400 Bad Request", 24) == 0) {
            std::string error_message = extractErrorMessage(header_str);
            std::cerr << error_message << std::endl;
            return false;
        }

        // Отправляем подтверждение если ошибок не возникло
        send(client_socket, std::to_string(header_received).c_str(), std::to_string(header_received).length(), 0);

        // Размер файла
        size_t data_size = parseSize(header_str);
        if (data_size == 0)
            return false;

        if (mode == 1) {
            // Создаем файл
            received_file.open(filename, std::ios::binary);
            if (!received_file.is_open()) {
                std::cerr << "Error creating file" << filename << std::endl;
                return false;
            }
        }

        ssize_t bytes_received;
        size_t total_bytes_received = 0;

        // Цикл для загрузки
        while (total_bytes_received < data_size) {
            char buffer[BUFFER_SIZE];
            bytes_received = recv(client_socket, buffer,
                                  std::min<size_t>(BUFFER_SIZE, data_size - total_bytes_received),
                                  0);
            //recv jnghfdkztn yt dct ,fqns vj;tn nj j;blfnm dct 1024 b gjnjv jnghfdkznm gjlndth;ltybt j ghbyznbb 1024 ,fqnjd

            if (bytes_received <= 0) {
                std::cerr << "Error receiving file content" << std::endl;
                if (mode == 1)
                    received_file.close();
                return false;
            }

            if (mode == 1)
                received_file.write(buffer, bytes_received);
            else
                std::cout.write(buffer, bytes_received);

            total_bytes_received += bytes_received;

            // Отправка количества полученных байт
            send(client_socket, std::to_string(bytes_received).c_str(), std::to_string(bytes_received).length(), 0);
        }

        if (mode == 1)
            received_file.close();
        return true;
    }

private:
    static int receiveBytes(int client_socket) {
        /*
         * Функция для получения количества принятых байт.
         */

        char bytesCount[10];

        if (const ssize_t bytes_received = recv(client_socket, bytesCount, sizeof(bytesCount), 0); bytes_received <= 0) {
            return -1; // Ошибка при получении подтверждения
        }

        return std::stoi(bytesCount);
    }

    static size_t parseSize(const std::string& header_str) {
        /*
         * Парсинг заголовочника, получаем размер файла.
        */
        const size_t content_length_pos = header_str.find("Content-Length:");

        if (content_length_pos == std::string::npos) {
            std::cerr << "Invalid file header" << std::endl;
            return 0;
        }

        const size_t content_length_end = header_str.find("\r\n", content_length_pos);

        if (content_length_end == std::string::npos) {
            std::cerr << "Invalid file header" << std::endl;
            return 0;
        }

        std::string content_length_str = header_str.substr(content_length_pos + 15,
                                                           content_length_end - (content_length_pos + 15));

        return std::stoul(content_length_str); // Размер файла
    }
};


#endif //UTILS_H

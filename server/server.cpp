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
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstdio>
#include <ctime>
#include <pwd.h>
#include <grp.h>
#include <string>

#include "server.h"
#include "../utils.cpp"
#include "../utils.h"


// static std::string getBinDir() {
//     char buffer[1024];
//     const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
//     if (len == -1) {
//         perror("readlink /proc/self/exe failed");
//         exit(-1);
//     }
//
//     buffer[len] = '\0';
//     auto binDir = std::string(buffer);
//
//     // Remove the binary filename from the path
//     if (const size_t lastSlashPos = binDir.find_last_of('/'); lastSlashPos != std::string::npos) {
//         binDir = binDir.substr(0, lastSlashPos);
//     }
//
//     return binDir;
// }

std::string Server::list_files(std::string path) {
    /*
     * Функция для получения листинга файлов.
     */
    DIR* thedirectory;
    dirent* thefile;
    struct stat thestat{};
    passwd* tf;
    group* gf;
    struct statvfs vfs{};

    off_t total_size = 0;

    std::string result;
    std::string server_path = "/mnt/c/Users/dana/uni/4course/sysprog2/pandora_box/client-server/server/bin/SERVER_/";

    // If path is empty, use the current working directory (SERVER_DIR)
    if (path.empty()) {
        path = server_path;
    } else {
        path = server_path + "/" + path;
    }

    // Get the block size
    if (statvfs(path.c_str(), &vfs) == -1) {
        perror("statvfs() error");
        return "";
    }

    thedirectory = opendir(path.c_str());

    while ((thefile = readdir(thedirectory)) != nullptr) {
        char buf[512];
        sprintf(buf, "%s/%s", path.c_str(), thefile->d_name);
        lstat(buf, &thestat);

        std::string link_result;
        switch (thestat.st_mode & S_IFMT) {
            case S_IFBLK: result += "b";
                break;
            case S_IFCHR: result += "c";
                break;
            case S_IFDIR: result += "d";
                break;
            case S_IFIFO: result += "p";
                break;
            case S_IFLNK: {
                result += "l";
                char link_target[1024];
                if (const ssize_t len = readlink(buf, link_target, sizeof(link_target) - 1); len != -1) {
                    link_target[len] = '\0';
                    link_result += " -> ";
                    link_result += link_target;
                }
                break;
            }
            case S_IFSOCK: result += "s";
                break;
            default: result += "-";
                break;
        }

        result += (thestat.st_mode & S_IRUSR) ? "r" : "-";
        result += (thestat.st_mode & S_IWUSR) ? "w" : "-";
        result += (thestat.st_mode & S_IXUSR) ? "x" : "-";
        result += (thestat.st_mode & S_IRGRP) ? "r" : "-";
        result += (thestat.st_mode & S_IWGRP) ? "w" : "-";
        result += (thestat.st_mode & S_IXGRP) ? "x" : "-";
        result += (thestat.st_mode & S_IROTH) ? "r" : "-";
        result += (thestat.st_mode & S_IWOTH) ? "w" : "-";
        result += (thestat.st_mode & S_IXOTH) ? "x" : "-";

        result += "\t";
        result += std::to_string(thestat.st_nlink);

        tf = getpwuid(thestat.st_uid);
        result += "\t";
        result += tf->pw_name;

        gf = getgrgid(thestat.st_gid);
        result += "\t";
        result += gf->gr_name;

        result += "\t";
        result += std::to_string(thestat.st_size);

        // Print the date in the format "Dec 13 11:55"
        char date[20];
        strftime(date, sizeof(date), "%b %d %H:%M", localtime(&thestat.st_mtime));
        result += "\t";
        result += date;

        result += "\t";
        result += thefile->d_name;
        result += link_result;
        result += "\n";

        total_size += thestat.st_size;
    }

    result += "\ntotal ";
    result += std::to_string(total_size / vfs.f_bsize);

    closedir(thedirectory);

    return result;
}

std::string Server::list_processes() {
    /*
     * Функция для получения списка процессов через файловую систему procfs (PID, путь до файла, аргументы, информация о файловых дескрипторах)
     */
    return std::string();
}

void* ServerHandler::handle_client(void* client_socket_ptr) {
    /*
     * Функция для обработки запросов от клиента.
     */
    const int client_socket = *static_cast<int *>(client_socket_ptr);
    delete static_cast<int *>(client_socket_ptr);

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

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // Обработка запроса в зависимости от протокола
        std::string response_message;

        if (strncmp(buffer, "GET /download", 13) == 0) {
            // Логика обработки запроса загрузки файлов с сервера
            std::cout << "Handling file download...\n";

            if (std::string filename = Utiliter::extractFilenameFromRequest(buffer); !filename.empty()) {
                if (Utiliter::sendFile(std::string(PATH_S) + filename, client_socket)) {
                    std::cout << "File " + filename + " sent successfully" << std::endl;
                    continue;
                }
                std::cerr << "Error sending file " << filename << "\n";
                response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 22\r\n\r\n" + std::string(
                                       "Error downloading file");
            } else {
                std::cerr << "Invalid download request.\n";
                response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 24\r\n\r\n" + std::string(
                                       "Invalid download request");
            }
        } else if (strncmp(buffer, "GET /list_files", 15) == 0) {
            // Логика обработки запроса листинга файлов
            std::cout << "Handling list files request...\n";
            if (auto files_list = Server::list_files(""); !files_list.empty()) {
                response_message = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(files_list.size()) +
                                   "\r\n\r\n" +
                                   files_list;
            } else {
                response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 24\r\n\r\n" + std::string(
                                       "Error getting list files");
            }
        } else if (strncmp(buffer, "GET /list_processes", 19) == 0) {
            // Логика обработки запроса списка процессов
            std::cout << "Handling list processes request...\n";
            if (std::string processes_list = Server::list_processes(); !processes_list.empty()) {
                response_message = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(processes_list.size()) +
                                   "\r\n\r\n" + processes_list;
            } else {
                response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" + std::string(
                                       "Error getting list processes");
            }
        } else if (strncmp(buffer, "POST /upload", 12) == 0) {
            // Логика обработки запроса выгрузки файлов на сервер
            std::cout << "Handling file upload...\n";

            if (std::string filename = Utiliter::extractFilenameFromRequest(buffer); !filename.empty()) {
                if (Utiliter::receiveFile(std::string(PATH_S) + filename, client_socket)) {
                    std::cout << "File received and saved as " << filename << std::endl;
                    continue;
                }
                std::cerr << "Error receiving file " << filename << "\n";
                continue;
            }
            std::cerr << "Invalid upload request.\n";
            response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 22\r\n\r\n" + std::string(
                                   "Invalid upload request");
        } else {
            // Нераспознанный запрос
            std::cerr << "Invalid request\n";
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
    sockaddr_in server_addr{}, client_addr{};
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
    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
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
    pollfd fds[MAX_CLIENTS + 1]; // +1 для слушающего сокета

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
                const int client_socket = accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_addr),
                                                 &addr_len);
                if (client_socket == -1) {
                    perror("Error accepting connection");
                    continue;
                }

                std::cout << "Connection accepted from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(
                    client_addr.sin_port) << "\n";

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

                if (const auto client_socket_ptr = new int(client_socket); pthread_create(&client_thread, nullptr,
                                                                               handle_client,
                                                                               (void *) client_socket_ptr) != 0) {
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
    // ReSharper disable once CppDFAUnreachableCode
    close(server_socket);
}

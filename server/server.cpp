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
#include <filesystem>

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

std::string Server::listFiles(std::string path) {
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

std::string Server::getListProcessesOutput(const std::vector<ProcessInfo>& process_info) {
    /*
     * Функция для преобразования значений структуры ProccessInfo в std::string.
     */
    if (process_info.empty()) {
        std::cerr << "No processes found." << std::endl;
        return "";
    }

    std::ostringstream stream;

    stream << std::left << std::setw(5) << "UID" << std::setw(8) << "PID" << std::setw(8) << "PPID" << std::setw(8)
            << "STATUS" << std::setw(10) << "TTY" << std::setw(10) << "CMD" << std::endl;
    stream << std::setfill('-') << std::setw(95) << "-" << std::setfill(' ') << std::endl;

    for (const auto& process: process_info) {
        stream << std::left << std::setw(5) << process.uid << std::setw(8) << process.pid << std::setw(8) <<
                process.ppid << std::setw(8) << process.st << std::setw(10) << process.tty_nr << std::setw(10) <<
                process.command << std::endl;
    }

    return stream.str();
}

std::vector<ProcessInfo> Server::listProcesses() {
    /*
     * Функция для получения списка процессов через файловую систему procfs (PID, путь до файла, аргументы, информация о файловых дескрипторах)
     */
    std::vector<ProcessInfo> processes;

    if (DIR* dir; (dir = opendir("/proc")) != nullptr) {
        dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR) {
                std::stringstream ssStat, ssStatus, ssCmdline;
                ssStat << "/proc/" << ent->d_name << "/stat";
                ssStatus << "/proc/" << ent->d_name << "/status";
                ssCmdline << "/proc/" << ent->d_name << "/cmdline";

                std::ifstream statFile(ssStat.str());
                std::ifstream statusFile(ssStatus.str());
                std::ifstream cmdlineFile(ssCmdline.str());
                if (statFile.is_open() && statusFile.is_open() && cmdlineFile.is_open()) {
                    ProcessInfo process;

                    process.pid = std::stoi(ent->d_name);

                    std::string line;
                    std::getline(statFile, line);

                    std::istringstream iss(line);

                    for (int i = 0; i < 7; ++i) {
                        iss >> process.tty_nr;
                    }

                    while (getline(statusFile, line)) {
                        if (line.compare(0, 6, "State:") == 0) {
                            process.st = line.substr(7)[0];
                        } else if (line.compare(0, 5, "PPid:") == 0) {
                            process.ppid = stoi(line.substr(6));
                        } else if (line.compare(0, 4, "Uid:") == 0) {
                            process.uid = stoi(line.substr(5));
                        }
                    }

                    // Skip kernel threads
                    if (process.pid > 0) {
                        // Read the command from /proc/[PID]/cmdline
                        process.command = getCommandLine(process.pid).substr(0, 128);
                        // getline(cmdlineFile, process.command);
                        // process.command = process.command.substr(0, 128);

                        processes.push_back(process);
                    }

                    statFile.close();
                    statusFile.close();
                    cmdlineFile.close();
                }
            }
        }
        closedir(dir);
    }

    return processes;
}

std::string Server::getProcessInfo(const std::string& pid) {
    std::vector<ProcessInfo> process_list = listProcesses();

    for (auto& process: process_list) {
        if (std::to_string(process.pid) == pid)
            return getListProcessesOutput({process});
    }

    return "";
}

std::string formatTimeMarks(const timespec timestamp) {
    tm tm_info{};
    localtime_r(&timestamp.tv_sec, &tm_info);

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);

    const int millisec = timestamp.tv_nsec / 1000000;
    std::sprintf(buffer + strlen(buffer), ".%03d", millisec);

    return buffer;
}

std::string Server::timeMarks(const std::string& filename) {
    const auto PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
    struct stat fileInfo{};

    std::string full_path = "../bin/" + PATH_S + filename;

    if (!std::filesystem::exists(full_path)) {
        std::cerr << "File " << filename << " doesn't exists" << std::endl;
        return "-1";
    }

    // Получаем информацию о файле
    if (stat(full_path.c_str(), &fileInfo) != 0) {
        std::cerr << "Error was occured while getting info about file" << std::endl;
        return "-2";
    }

    std::ostringstream result;

    result << "File: " << filename << std::endl << "Access: " << formatTimeMarks(fileInfo.st_atim) << std::endl
            << "Modify: " << formatTimeMarks(fileInfo.st_mtim) << std::endl << "Change: " << formatTimeMarks(
                fileInfo.st_ctim) << std::endl;

    return result.str();
}

std::string Server::getCommandLine(int pid) {
    std::ostringstream path;
    path << "/proc/" << pid << "/cmdline";

    std::ifstream cmdlineFile(path.str());
    if (!cmdlineFile.is_open()) {
        // Ошибка при открытии файла
        return {};
    }

    // Чтение содержимого файла (аргументы командной строки)
    std::ostringstream cmdlineContent;
    cmdlineContent << cmdlineFile.rdbuf();
    cmdlineFile.close();

    // Разделение аргументов
    std::istringstream cmdlineStream(cmdlineContent.str());
    std::vector<std::string> arguments;
    std::string argument;

    while (std::getline(cmdlineStream, argument, '\0')) {
        arguments.push_back(argument);
    }

    std::string result;

    if (!arguments.empty()) {
        for (const auto& arg: arguments) {
            result += arg;
            result += " ";
        }
    } else {
        std::cerr << "Error reading command line arguments for PID " << pid << std::endl;
    }

    return result;
}

bool Server::sendProcessList(const int clientSocket, const std::string& processesString) {
    const auto BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");

    size_t totalBytesSent = 0;

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\nContent-Length: " << processesString.length() << "\r\n\r\n";

    // Отправляем заголовочник с размером файла
    if (Utiliter::sendAll(clientSocket, header.str().c_str(), header.str().length()) == -1) {
        std::cerr << "Error sending header" << std::endl;
        return false;
    }

    while (totalBytesSent < processesString.length()) {
        const size_t bytesToSend = std::min(static_cast<size_t>(BUFFER_SIZE),
                                            processesString.length() - totalBytesSent);
        const ssize_t sentBytes = send(clientSocket, processesString.c_str() + totalBytesSent, bytesToSend, 0);

        if (sentBytes == -1) {
            std::cerr << "Error sending data" << std::endl;
            return false;
        }

        totalBytesSent += static_cast<size_t>(sentBytes);
    }

    return true;
}

void* ServerHandler::handleClient(void* client_socket_ptr) {
    /*
     * Функция для обработки запросов от клиента.
     */

    auto PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
    const auto BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");

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
                if (Utiliter::sendFile(PATH_S + filename, client_socket)) {
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
            if (auto files_list = Server::listFiles(""); !files_list.empty()) {
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
            if (std::string processesString = Server::getListProcessesOutput(Server::listProcesses()); !processesString.
                empty()) {
                if (Utiliter::sendString(processesString, client_socket)) {
                    std::cout << "Process list sent succesfully" << std::endl;
                    continue;
                }
            } else {
                std::cerr << "Error sending process list" << "\n";
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
        } else if (strncmp(buffer, "GET /process_info", 17) == 0) {
            std::cout << "Handling process info request...\n";

            if (std::string pid = Utiliter::extractPidFromRequest(buffer); !pid.empty()) {
                if (std::string processesString = Server::getProcessInfo(pid); !processesString.empty()) {
                    if (Utiliter::sendString(processesString, client_socket)) {
                        std::cout << "Process info sent succesfully" << std::endl;
                        continue;
                    }
                } else {
                    std::cerr << "Process with this PID does not exist" << "\n";
                    response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" + std::string(
                                           "Process with this PID does not exist");
                }
            }
        } else if (strncmp(buffer, "GET /time_marks", 15) == 0) {
            std::cout << "Handling time marks request...\n";

            if (std::string filename = Utiliter::extractFilenameFromRequest(buffer); !filename.empty()) {
                if (std::string timeMarksOutput = Server::timeMarks(filename); !timeMarksOutput.empty()) {
                    if (timeMarksOutput == "-1")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("File " + filename + " doesn't exists");
                    else if (timeMarksOutput == "-2")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("Error getting time marks");
                    else {
                        if (Utiliter::sendString(timeMarksOutput, client_socket)) {
                            std::cout << "Time marks sent succesfully" << std::endl;
                            continue;
                        }
                    }
                }
            }
        } else {
            // Нераспознанный запрос
            std::cerr << "Invalid request\n";
            response_message = "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\n\r\nInvalid request";
        }

        // Отправляем ответ клиенту
        send(client_socket, response_message.c_str(), response_message.length(), 0);
    }

    // Закрываем сокет клиента
    close(client_socket);

    pthread_exit(nullptr);
}

void ServerHandler::runServer() {
    auto SERVER_IP = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "connection", "SERVER_IP");
    const auto PORT = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "PORT");
    const auto MAX_CLIENTS = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "MAX_CLIENTS");

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
                                                                               handleClient,
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

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
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

#include "server.h"
#include "../utils.cpp"
#include "../utils.h"


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

    std::ostringstream output;
    std::string server_path = "/mnt/c/Users/dana/uni/4course/sysprog2/pandora_box/client-server/server/bin/SERVER_/";

    if (path.empty()) {
        path = server_path;
    } else {
        path = server_path + "/" + path;
    }

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
            case S_IFBLK: output << "b";
                break;
            case S_IFCHR: output << "c";
                break;
            case S_IFDIR: output << "d";
                break;
            case S_IFIFO: output << "p";
                break;
            case S_IFLNK: {
                output << "l";
                char link_target[1024];
                if (const ssize_t len = readlink(buf, link_target, sizeof(link_target) - 1); len != -1) {
                    link_target[len] = '\0';
                    link_result += " -> ";
                    link_result += link_target;
                }
                break;
            }
            case S_IFSOCK: output << "s";
                break;
            default: output << "-";
                break;
        }

        output << ((thestat.st_mode & S_IRUSR) ? "r" : "-");
        output << ((thestat.st_mode & S_IWUSR) ? "w" : "-");
        output << ((thestat.st_mode & S_IXUSR) ? "x" : "-");
        output << ((thestat.st_mode & S_IRGRP) ? "r" : "-");
        output << ((thestat.st_mode & S_IWGRP) ? "w" : "-");
        output << ((thestat.st_mode & S_IXGRP) ? "x" : "-");
        output << ((thestat.st_mode & S_IROTH) ? "r" : "-");
        output << ((thestat.st_mode & S_IWOTH) ? "w" : "-");
        output << ((thestat.st_mode & S_IXOTH) ? "x" : "-");

        output << " ";
        output << std::to_string(thestat.st_nlink);

        tf = getpwuid(thestat.st_uid);
        output << " ";
        output << tf->pw_name;

        gf = getgrgid(thestat.st_gid);
        output << " ";
        output << gf->gr_name;

        output << " ";
        output << std::to_string(thestat.st_size);
        output << " ";

        char date[20];
        strftime(date, sizeof(date), "%b %d %H:%M", localtime(&thestat.st_mtime));
        output << " ";
        output << date;

        output << " ";
        output << thefile->d_name;
        output << link_result;
        output << "\n";

        total_size += thestat.st_size;
    }

    output << "\ntotal ";
    output << std::to_string(total_size / vfs.f_bsize);

    closedir(thedirectory);

    return output.str();
}

std::string Server::getFileDescriptors(int pid) {
    struct stat thestat{};
    std::ostringstream output;
    dirent* thefile;

    std::ostringstream path;
    path << "/proc/" << pid << "/fd/";
    DIR* dir = opendir(path.str().c_str());

    while ((thefile = readdir(dir)) != nullptr) {
        char buf[512];
        sprintf(buf, "%s/%s", path.str().c_str(), thefile->d_name);
        lstat(buf, &thestat);

        switch (thestat.st_mode & S_IFMT) {
            case S_IFLNK: {
                char link_target[1024];
                if (const ssize_t len = readlink(buf, link_target, sizeof(link_target) - 1); len != -1) {
                    std::string link_result;
                    link_target[len] = '\0';
                    link_result += " -> ";
                    link_result += link_target;

                    output << thefile->d_name;
                    output << link_result;
                    output << " ";
                }
                break;
            }
        }
    }

    return output.str();
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
            << "STATUS" << std::setw(10) << "CMD" << " " << std::setw(5) << "FD" << std::endl;
    stream << std::setfill('-') << std::setw(158) << "-" << std::setfill(' ') << std::endl;

    for (const auto& [uid, pid, ppid, st, command, fd]: process_info) {
        stream << std::left << std::setw(5) << uid << std::setw(8) << pid << std::setw(8) <<
                ppid << std::setw(8) << st << std::setw(10) <<
                command << " " << std::setw(5) << fd << std::endl;
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
                if (std::ifstream cmdlineFile(ssCmdline.str());
                    statFile.is_open() && statusFile.is_open() && cmdlineFile.is_open()) {
                    ProcessInfo process;

                    process.pid = std::stoi(ent->d_name);

                    std::string line;
                    std::getline(statFile, line);

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
                        process.command = getCommandLine(process.pid).substr(0, 64);
                        process.fd = getFileDescriptors(process.pid).substr(0, 64);

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
    /*
     * Функция, возвращающая информацию об одном процессе.
     */
    for (const std::vector<ProcessInfo> process_list = listProcesses(); auto& process: process_list) {
        if (std::to_string(process.pid) == pid)
            return getListProcessesOutput({process});
    }

    return "";
}

std::string formatTimeMarks(const timespec timestamp) {
    /*
     * Функция для форматирования вывода функции timeMarks().
     */
    tm tm_info{};
    localtime_r(&timestamp.tv_sec, &tm_info);

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);

    const int millisec = timestamp.tv_nsec / 1000000;
    std::sprintf(buffer + strlen(buffer), ".%03d", millisec);

    return buffer;
}

std::string Server::timeMarks(const std::string& filename) {
    /*
     *Функция для получения временных меток файла.
     */
    struct stat fileInfo{};

    const std::string full_path = "../bin/" + PATH_S + filename;

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
    /*
     * Функция для получения полного имении файла процесса и его аргументов
     */
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

std::string Server::executeCommand(std::string& filename) const {
    /*
     * Функция для получения стандартного вывода исполняемого файла
     */
    std::filesystem::path path(filename);

    // Проверяем, есть ли символ '/' в пути
    if (path.string().find('/') == std::string::npos) {
        // Если нет, добавляем префикс "SERVER_/"
        path = std::filesystem::path(PATH_S) / path;
    }

    filename = path.string();

    if (!std::filesystem::exists(filename)) {
        std::cerr << "File " << filename << " doesn't exists" << std::endl;
        return "-1";
    }

    std::array<char, 128> buffer{};
    std::string result;

    // Используем popen для выполнения команды и получения её вывода
    FILE* pipe = popen(filename.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error opening pipe" << std::endl;
        return "-2";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    if (const auto return_code = pclose(pipe); return_code != 0) {
        std::cerr << "Error was occured while closing the file" << std::endl;
        return "-3";
    }

    return result;
}

size_t Server::getOffset(const std::string& filename, int client_socket) const {
    // Отправляем клиенту информацию об имени файла
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n" << "?filename=" << filename << " HTTP/1.1\r\n\r\n";

    // Отправляем заголовочник с именем файла
    if (send(client_socket, header.str().c_str(), header.str().length(), 0) == -1) {
        std::cerr << "Error sending header" << std::endl;
        return false;
    }

    // Получаем количество доставленных байт
    if (const int receivedBytes = Utiliter::receiveBytes(client_socket); receivedBytes == -1) {
        std::cerr << "Error receiving byts count" << std::endl;
        return false;
    }

    // Получаем заголовочник с размером файла клиента
    char file_size[BUFFER_SIZE];
    const ssize_t header_received = recv(client_socket, file_size, BUFFER_SIZE, 0);

    if (header_received <= 0) {
        std::cerr << "Error receiving file header" << std::endl;
        return false;
    }

    // Отправляем подтверждение если ошибок не возникло
    send(client_socket, std::to_string(header_received).c_str(), std::to_string(header_received).length(), 0);

    const std::string header_str(file_size, header_received);
    return Utiliter::parseSize(header_str);
}

bool Server::updateUndownloadedFiles(const std::vector<std::string>& files, int client_socket) {
    Utiliter ut;
    Server sr;

    // Отправляем клиенту информацию о количестве файлов для докачки
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\nContent-Length: " << files.size() << "\r\n\r\n";

    // Отправляем заголовочник с количеством файлов для докачки
    if (send(client_socket, header.str().c_str(), header.str().length(), 0) == -1) {
        std::cerr << "Error sending header" << std::endl;
        return false;
    }

    // Получаем количество доставленных байт
    if (const int receivedBytes = Utiliter::receiveBytes(client_socket); receivedBytes == -1) {
        std::cerr << "Error receiving byts count" << std::endl;
        return false;
    }

    if (files.empty())
        return true;

    for (const auto& filename: files) {
        std::string filename_ = filename.substr(0, filename.size() - 30);
        if (ut.send_(PATH_S + filename_, client_socket, 1, sr.getOffset(filename, client_socket))) {
            std::cout << "File " + filename + " downloaded completely" << std::endl;
            continue;
        }
        std::cerr << "Error sending file " << filename << "\n";
        std::string response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 22\r\n\r\n" + std::string(
                                           "Error downloading file");

        send(client_socket, response_message.c_str(), response_message.length(), 0);
    }

    return true;
}

void* ServerHandler::handleClient(void* client_socket_ptr) {
    /*
     * Функция для обработки запросов от клиента.
     */
    auto PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
    auto BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");

    Server sr;
    Utiliter ut;

     // const int client_socket = *static_cast<int *>(client_socket_ptr);
     // delete static_cast<int *>(client_socket_ptr);
    int client_socket = *static_cast<int*>(client_socket_ptr);

    char buffer[BUFFER_SIZE];

    std::vector<std::string> files = ut.checkForUndownloadedFiles();
    if (!sr.updateUndownloadedFiles(files, client_socket)) {
        std::cerr << "Error updating undownloaded files" << std::endl;
    }

    while (true) {
        // Принимаем запрос от клиента
        const ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Если recv возвращает 0 или отрицательное значение, клиент отключился
            if (bytes_received == 0) {
                std::cout << "Client disconnected.\n";
                break;
            }
            continue;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // Обработка запроса в зависимости от протокола
        std::string response_message;

        if (strncmp(buffer, "GET /download", 13) == 0) {
            // Логика обработки запроса загрузки файлов с сервера
            std::cout << "Handling file download...\n";

            if (std::string filename = Utiliter::extractFilenameFromRequest(buffer); !filename.empty()) {
                if (ut.send_(PATH_S + filename, client_socket, 1, 0)) {
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
                if (ut.send_(files_list, client_socket, 2, 0)) {
                    std::cout << "Process list sent succesfully" << std::endl;
                    continue;
                }
            } else {
                std::cerr << "Error sending list files" << "\n";
                response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 24\r\n\r\n" + std::string(
                                       "Error getting list files");
            }
        } else if (strncmp(buffer, "GET /list_processes", 19) == 0) {
            // Логика обработки запроса списка процессов
            std::cout << "Handling list processes request...\n";
            if (std::string processesString = Server::getListProcessesOutput(Server::listProcesses()); !
                processesString.
                empty()) {
                if (ut.send_(processesString, client_socket, 2, 0)) {
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
                if (ut.receive_(std::string(PATH_S) + filename, client_socket, 1, 0)) {
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
                    if (ut.send_(processesString, client_socket, 2, 0)) {
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
                if (std::string timeMarksOutput = sr.timeMarks(filename); !timeMarksOutput.empty()) {
                    if (timeMarksOutput == "-1")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("File " + filename + " doesn't exists");
                    else if (timeMarksOutput == "-2")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("Error getting time marks");
                    else {
                        if (ut.send_(timeMarksOutput, client_socket, 2, 0)) {
                            std::cout << "Time marks sent succesfully" << std::endl;
                            continue;
                        }
                    }
                }
            }
        } else if (strncmp(buffer, "GET /execute", 12) == 0) {
            std::cout << "Handling execute file request...\n";

            if (std::string filename = Utiliter::extractFilenameFromRequest(buffer); !filename.empty()) {
                if (std::string executeOutput = sr.executeCommand(filename); !executeOutput.empty()) {
                    if (executeOutput == "-1")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("File " + filename + " doesn't exist");
                    else if (executeOutput == "-2")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("Error opening pipe");
                    else if (executeOutput == "-3")
                        response_message = "HTTP/1.1 500 Internal Error\r\nContent-Length: 28\r\n\r\n" +
                                           std::string("Error getting time marks");
                    else {
                        if (ut.send_(executeOutput, client_socket, 2, 0)) {
                            std::cout << "Output sent succesfully" << std::endl;
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

void ServerHandler::runServer(const Server& sr) {
    int client_socket;
    sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::vector<Client*> clients;
    pollfd fds[sr.MAX_CLIENTS + 1];

    // Создание сокета
    const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(sr.PORT);

    // Привязка сокета
    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    // Прослушивание сокета
    if (listen(server_socket, 5) == -1) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << sr.PORT << "...\n";

    // Инициализация структуры pollfd для серверного сокета
    fds[0].fd = server_socket;
    fds[0].events = POLLIN;

    while (true) {
        // Мультиплексирование с poll
        int poll_result = poll(fds, clients.size() + 1, -1);

        if (poll_result < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            // Принятие нового подключения
            client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);

            // Создание нового потока для обработки клиента
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handleClient, static_cast<void*>(&client_socket));
            pthread_detach(thread_id);
        }

        for (size_t i = 0; i < clients.size() && poll_result > 0; i++) {
            if (fds[i + 1].revents & POLLIN) {
                // Обработка ввода для существующих клиентов
                // В данном случае ничего не делаем, просто читаем и отправляем обратно
                poll_result--;
            }
        }
    }

    // Закрытие сокета сервера
    close(server_socket);
}

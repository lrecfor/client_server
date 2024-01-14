//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>

#include "../ConfigHandler.h"

struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    std::string st;
    std::string command;
    std::string fd;
};

class Server;

class ServerHandler {
public:
    static void* handleClient(void* client_socket_ptr);

    static void runServer(const Server &sr);
};


class Server {
public:
    int PORT;
    int MAX_CLIENTS;
    int BUFFER_SIZE;
    std::string PATH_S;
    std::string SERVER_IP;

    Server() {
        PORT = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "PORT");
        PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
        MAX_CLIENTS = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "MAX_CLIENTS");
        BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");
        SERVER_IP = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "connection", "SERVER_IP");
    }

    static std::string listFiles(std::string path);

    static std::string getProcessInfo(const std::string& pid);

    std::string timeMarks(const std::string& filename);

    static std::string getCommandLine(int pid);

    bool sendProcessList(int clientSocket, const std::string& processesString);

    std::string executeCommand(std::string& command) const;

    static std::string getListProcessesOutput(const std::vector<ProcessInfo>& process_info);

    static std::vector<ProcessInfo> listProcesses();

    static auto getFileDescriptors(int pid) -> std::string;

    friend class ServerHandler;
};


#endif //SERVER_H

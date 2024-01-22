//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <string>
#include <vector>

#include "../ConfigHandler.h"
#include "../client/client.h"

struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    std::string st;
    std::string command;
    std::string fd;
};


class Server;
class Client;

class ServerHandler {

public:
    static bool handleClient(void* client_socket_ptr);

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

    std::string timeMarks(const std::string& filename) const;

    static std::string getCommandLine(int pid);

    std::string executeCommand(std::string& command) const;

    size_t getOffset(const std::string& filename, int client_socket) const;

    static std::string getListProcessesOutput(const std::vector<ProcessInfo>& process_info);

    static std::vector<ProcessInfo> listProcesses();

    static auto getFileDescriptors(int pid) -> std::string;

    bool checkingForUpdates(int client_socket);

    friend class ServerHandler;
};



#endif //SERVER_H

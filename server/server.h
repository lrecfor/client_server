//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <sstream>
#include <vector>

#include "../ConfigHandler.h"

struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    std::string st;
    std::string command;
};


class Server {
public:
    static std::string listFiles(std::string path);

    static std::string getProcessInfo(const std::string& pid);

    static std::string timeMarks(const std::string& filename);

    static std::string getCommandLine(int pid);

    static bool sendProcessList(int clientSocket, const std::string& processesString);

    static std::string executeCommand(std::string& command);

    static std::string getListProcessesOutput(const std::vector<ProcessInfo>& process_info);

    static std::vector<ProcessInfo> listProcesses();
};


class ServerHandler {
public:
    static void* handleClient(void* client_socket_ptr);

    static void runServer();
};


#endif //SERVER_H

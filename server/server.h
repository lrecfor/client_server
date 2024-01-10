//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <sstream>
#include <vector>


struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    long tty_nr;
    std::string st;
    std::string command;
};


class Server {
public:
    static std::string listFiles(std::string path);

    static std::string listProcesses();

    static std::string getCommandLine(int pid);

    static bool sendProcessList(int clientSocket, const std::string& processesString);

    static std::string getListProcessesOutput(const std::vector<ProcessInfo>& process_info);
};


class ServerHandler {
public:
    static void* handleClient(void* client_socket_ptr);

    static void runServer();
};


#endif //SERVER_H

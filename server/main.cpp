//
// Created by dana on 05.01.2024.
//

#include "server.h"


// int main() {
//     ServerHandler::run_server();
//
//     return 0;
// }

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <iterator>

using namespace std;

struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    long tty_nr;
    string st;
    string startTime;
    string tty;
    string command;
};

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

vector<ProcessInfo> getProcessesInfo() {
    vector<ProcessInfo> processes;

    DIR *dir;
    dirent *ent;

    if ((dir = opendir("/proc")) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR) {
                stringstream ssStat, ssStatus, ssCmdline;
                ssStat << "/proc/" << ent->d_name << "/stat";
                ssStatus << "/proc/" << ent->d_name << "/status";
                ssCmdline << "/proc/" << ent->d_name << "/cmdline";

                ifstream statFile(ssStat.str());
                ifstream statusFile(ssStatus.str());
                ifstream cmdlineFile(ssCmdline.str());
                if (statFile.is_open() && statusFile.is_open() && cmdlineFile.is_open()) {
                    ProcessInfo process;

                    process.pid = stoi(ent->d_name);

                    string line;
                    std::getline(statFile, line);

                    std::istringstream iss(line);
                    // Считываем нужные поля из строки
                    for (int i = 0; i < 6; ++i) {
                        std::string temp;
                        iss >> temp;
                    }

                    iss >> process.tty_nr;

                    //std::string line;
                    while (getline(statusFile, line)) {
                        if (line.compare(0, 6, "State:") == 0) {
                            process.st = line.substr(7)[0];
                        }
                        else if (line.compare(0, 5, "PPid:") == 0) {
                            process.ppid = stoi(line.substr(6));
                        }
                        else if (line.compare(0, 4, "Uid:") == 0) {
                            process.uid = stoi(line.substr(5));
                        }
                    }

                    // Skip kernel threads
                    if (process.pid > 0) {
                        // Calculate CPU usage (utime + stime)
                        for (int i = 0; i < 12; ++i) {
                            statFile >> process.startTime;
                        }

                        // Skip the next fields
                        for (int i = 0; i < 2; ++i) {
                            statFile >> process.tty;
                        }

                        // Read the command from /proc/[PID]/cmdline
                        getline(cmdlineFile, process.command);

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

int main() {
    vector<ProcessInfo> processes = getProcessesInfo();

    if (processes.empty()) {
        cerr << "No processes found." << endl;
        return 1;
    }

    std::cout << left << setw(5) << "UID" << setw(8) << "PID" << setw(8) << "PPID" << setw(8) << "STATUS" << setw(10) << "STIME" << setw(10) << "TTY" << setw(50) << "CMD" << std::endl;
    std::cout << setfill('-') << setw(95) << "-" << setfill(' ') << std::endl;

    for (const auto& process : processes) {
        std::cout << left << setw(5) << process.uid << setw(8) << process.pid << setw(8) << process.ppid << setw(8) << process.st
             << setw(10) << process.startTime << setw(10) << process.tty << setw(50) << process.command << std::endl;
    }

    return 0;
}

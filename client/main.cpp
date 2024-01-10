//
// Created by dana on 05.01.2024.
//

#include "client.h"

void fun() {
    std::ifstream cFile("../config.cfg");
    if (cFile.is_open()) {
        std::string line;
        while (getline(cFile, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                       line.end());
            if (line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            std::cout << name << " " << value << '\n';
        }
    } else {
        std::cerr << "Couldn't open config file for reading.\n";
    }
}


int main() {
    Client::runClient();

    return 0;
}

// Примеры запросов
// "GET /download?filename=FILE11.txt HTTP/1.1\r\n\r\n";
// "GET /list_files HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
// "GET /list_processes HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
// "POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n";

/*
 * GET /download?filename=FILE1.txt HTTP/1.1\r\n\r\n
 * GET /download?filename=FILE11.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=FILE2.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n
 */

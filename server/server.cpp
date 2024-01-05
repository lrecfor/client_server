//
// Created by dana on 05.01.2024.
//

#include "server.h"

bool Server::upload_files(std::string filename) {
    return true;
}

std::vector<std::string> Server::list_files(std::string directory) {
    return std::vector<std::string>({"first, second"});
}

std::vector<std::string> Server::list_processes(std::string directory) {
    return std::vector<std::string>({"third, fourth"});
}

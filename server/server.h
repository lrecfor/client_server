//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>

class Server {

public:
    bool upload_files(std::string filename);
    std::vector<std::string> list_files(std::string directory);
    std::vector<std::string> list_processes(std::string directory);

};



#endif //SERVER_H

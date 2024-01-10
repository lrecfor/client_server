//
// Created by dana on 05.01.2024.
//

#include "client.h"


int main() {
    Client::runClient();

    return 0;
}

// #include <iostream>
//
// int main(int argc, char *argv[]) {
//     std::cout << "Number of command line arguments: " << argc << std::endl;
//
//     std::cout << "Command line arguments:" << std::endl;
//     for (int i = 0; i < argc; ++i) {
//         std::cout << "Argument " << i << ": " << argv[i] << std::endl;
//     }
//
//     while(true) {
//         continue;
//     }
//
//     return 0;
// }


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

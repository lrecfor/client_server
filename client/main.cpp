//
// Created by dana on 05.01.2024.
//

#include "client.h"


int main() {
    Client::run_client();

    return 0;
}

    // Примеры запросов
    // "GET /download?filename=FILE1.txt HTTP/1.1\r\n\r\n";
    // "GET /list_files HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    // "GET /list_processes HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    // "POST /upload?filename=PHOTO7878.jpg HTTP/1.1\r\n\r\n";
//
// Created by dana on 05.01.2024.
//

#include "client.h"


int main() {
    Client::run_client();

    return 0;
}


    // Примеры запросов
    // const auto upload_request = "GET /download?filename=FILE1.txt HTTP/1.1\r\n\r\n";
    // const auto list_files_request = "GET /list_files HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    // const auto list_processes_request = "GET /list_processes HTTP/1.1\r\nHost: server_ip:port\r\n\r\n";
    //
    // // Отправка запросов
    // std::cout << "Sending upload request...\n";
    // Client::send_request(upload_request);
    //
    // std::cout << "Sending list files request...\n";
    // Client::send_request(list_files_request);
    //
    // std::cout << "Sending list processes request...\n";
    // Client::send_request(list_processes_request);
    //
    // return 0;

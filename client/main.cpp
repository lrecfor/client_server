//
// Created by dana on 05.01.2024.
//

#include "client.h"


int main() {
    Client::runClient();

    return 0;
}


// Примеры запросов
// "GET /download?filename=FILE11.txt HTTP/1.1\r\n\r\n";
// "GET /list_files HTTP/1.1\r\n";
// "GET /list_processes HTTP/1.1\r\n";
// "POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n";
// "GET /process_info?pid=2"
// "GET /time_marks?filename=FILE1.txt HTTP/1.1\r\n\r\n"
// "GET /execute?filename=run HTTP/1.1\r\n\r\n"

/*
 * GET /download?filename=FILE1.txt HTTP/1.1\r\n\r\n
 * GET /download?filename=FILE11.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=FILE2.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n
 */

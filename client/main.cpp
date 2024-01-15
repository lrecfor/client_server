//
// Created by dana on 05.01.2024.
//

#include "client.h"


int main() {
    const Client cl;
    cl.runClient();

    return 0;
}


// Примеры запросов
// GET /download?filename=BIGBOY.txt HTTP/1.1\r\n\r\n
// GET /list_files HTTP/1.1\r\n\r\n
// GET /list_processes HTTP/1.1\r\n\r\n
// POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n
// GET /process_info?pid=2 HTTP/1.1\r\n\r\n
// GET /time_marks?filename=FILE1.txt HTTP/1.1\r\n\r\n
// GET /execute?filename=run HTTP/1.1\r\n\r\n
// GET /execute?filename=ls -lah HTTP/1.1\r\n\r\n

/*
 * GET /download?filename=VIDEO.MP4 HTTP/1.1\r\n\r\n
 * GET /download?filename=BRONETRANSPORTER.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=BRONETRANSPORTER.txt HTTP/1.1\r\n\r\n
 * POST /upload?filename=FILE41.txt HTTP/1.1\r\n\r\n
 */

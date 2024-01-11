//
// Created by dana on 05.01.2024.
//

// #include "server.h"
//
// int main() {
//     ServerHandler::runServer();
//
//     return 0;
// }

#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <filesystem>



int main() {
    std::string filename = "SERVER_/BIGBOY.txt"; // Замените на фактический путь к файлу
    std::cout << printFileTimestamps(filename);

    return 0;
}


// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <filesystem>
//
// void printFileDescriptors(int pid) {
//     std::ostringstream path;
//     path << "/proc/" << pid << "/fd/";
//
//     std::filesystem::directory_iterator it(path.str());
//     std::filesystem::directory_iterator end;
//
//     for (; it != end; ++it) {
//         std::ostringstream fdPath;
//         fdPath << *it;
//         std::cout << "File Descriptor: " << fdPath.str() << std::endl;
//
//         // You can add more information about the file descriptor if needed
//     }
// }
//
// int main() {
//     int pid = 2; // Получаем PID текущего процесса
//
//     std::cout << "File Descriptors for PID " << pid << ":" << std::endl;
//     printFileDescriptors(pid);
//
//     return 0;
// }

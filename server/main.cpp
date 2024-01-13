//
// Created by dana on 05.01.2024.
//

#include "server.h"

int main() {
    ServerHandler::runServer();

    return 0;
}

// #include <iostream>
// #include <filesystem>
// #include <unistd.h>
// #include <cstring>
// #include <limits.h>
// #include <iostream>
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
//
//         char targetPath[PATH_MAX];
//         ssize_t bytesRead = readlink(fdPath.str().c_str(), targetPath, sizeof(targetPath) - 1);
//
//         if (bytesRead != -1) {
//             targetPath[bytesRead] = '\0';
//             std::cout << "File Descriptor: " << fdPath.str() << " -> " << targetPath << std::endl;
//         } else {
//             std::cerr << "Error reading link for: " << fdPath.str() << std::endl;
//         }
//     }
// }
//
// int main() {
//     int pid = 2;  // Replace with the actual process ID you want to inspect
//     printFileDescriptors(pid);
//
//     return 0;
// }


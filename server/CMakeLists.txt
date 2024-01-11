cmake_minimum_required(VERSION 3.22)
project(client_server)

set(CMAKE_CXX_STANDARD 20)

include_directories(/usr/include)

add_executable(server main.cpp
        server.cpp
        server.h
        ../ConfigHandler.h
)

target_link_libraries(server config++)
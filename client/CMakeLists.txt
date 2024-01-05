cmake_minimum_required(VERSION 3.22)
project(client_server)

set(CMAKE_CXX_STANDARD 20)

add_executable(client main.cpp
        client.h
)
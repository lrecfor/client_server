//
// Created by dana on 05.01.2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <string>
#include <vector>

#include "../ConfigHandler.h"
#include "../client/client.h"

struct ProcessInfo {
    int uid;
    int pid;
    int ppid;
    std::string st;
    std::string command;
    std::string fd;
};


inline int MAX_THREADS = 10;


class Server;
class Client;

class ThreadPool {

    struct Task {
        std::function<void(void*)> task;
        void* client_socket_ptr;

        Task(std::function<void(void*)> task, void* client_socket_ptr)
            : task(std::move(task)), client_socket_ptr(client_socket_ptr) {}

        void execute() {
            task(client_socket_ptr);
        }
    };


    std::vector<pthread_t> threads;
    std::queue<Task> tasks;

    pthread_mutex_t queueMutex;
    pthread_cond_t condition;
    bool stop;
public:
    ThreadPool() : stop(false) {
        pthread_mutex_init(&queueMutex, nullptr);
        pthread_cond_init(&condition, nullptr);

        for (int i = 0; i < MAX_THREADS; ++i) {
            pthread_t thread;
            pthread_create(&thread, nullptr, &ThreadPool::workerThread, this);
            threads.push_back(thread);
        }
    }

    ~ThreadPool() {
        {
            pthread_mutex_lock(&queueMutex);
            stop = true;
            pthread_mutex_unlock(&queueMutex);
        }
        pthread_cond_broadcast(&condition);

        for (pthread_t& thread : threads) {
            pthread_join(thread, nullptr);
        }

        pthread_mutex_destroy(&queueMutex);
        pthread_cond_destroy(&condition);
    }

    void addTask(void* client_socket_ptr, std::function<void(void*)> task) {
        pthread_mutex_lock(&queueMutex);
        tasks.emplace(task, client_socket_ptr);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&queueMutex);
    }


private:
    static void* workerThread(void* arg) {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        pool->workerThread();
        return nullptr;
    }

    void workerThread() {
        while (true) {
            Task task(nullptr, nullptr);  // Объект Task с пустыми параметрами
            {
                pthread_mutex_lock(&queueMutex);
                while (tasks.empty() && !stop) {
                    pthread_cond_wait(&condition, &queueMutex);
                }

                if (stop && tasks.empty()) {
                    pthread_mutex_unlock(&queueMutex);
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
                pthread_mutex_unlock(&queueMutex);
            }

            task.execute();  // Вызываем метод execute без параметра
        }
    }

};

class ServerHandler {
public:
    static bool handleClient(void* client_socket_ptr);

    static void handle_client(void* client_socket_ptr);

    static void runServer(const Server &sr);
};


class Server {
public:
    int PORT;
    int RUN_TYPE;
    int MAX_CLIENTS;
    int BUFFER_SIZE;
    std::string PATH_S;
    std::string SERVER_IP;

    Server() {
        PORT = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "PORT");
        RUN_TYPE = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "RUN_TYPE");
        PATH_S = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "send_const", "PATH_S");
        MAX_CLIENTS = ConfigHandler::getConfigValue<int>("../../config.cfg", "connection", "MAX_CLIENTS");
        BUFFER_SIZE = ConfigHandler::getConfigValue<int>("../../config.cfg", "send_const", "BUFFER_SIZE");
        SERVER_IP = ConfigHandler::getConfigValue<std::string>("../../config.cfg", "connection", "SERVER_IP");
    }

    static std::string listFiles(std::string path);

    static std::string getProcessInfo(const std::string& pid);

    std::string timeMarks(const std::string& filename) const;

    static std::string getCommandLine(int pid);

    std::string executeCommand(std::string& command) const;

    size_t getOffset(const std::string& filename, int client_socket) const;

    static std::string getListProcessesOutput(const std::vector<ProcessInfo>& process_info);

    static std::vector<ProcessInfo> listProcesses();

    static auto getFileDescriptors(int pid) -> std::string;

    bool updateUnsendedFiles(int client_socket) const;

    friend class ServerHandler;
};



#endif //SERVER_H

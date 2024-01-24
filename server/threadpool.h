//
// Created by dana on 24.01.2024.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <functional>
#include <queue>
#include <string>
#include <vector>


class ThreadPool {

    struct Task {
        std::function<void(void*)> task;
        void* client_socket_ptr;

        Task(std::function<void(void*)> task, void* client_socket_ptr)
            : task(std::move(task)), client_socket_ptr(client_socket_ptr) {}

        void execute() const {
            task(client_socket_ptr);
        }
    };


    std::vector<pthread_t> threads;
    std::queue<Task> tasks;

    pthread_mutex_t queueMutex{};
    pthread_cond_t condition{};
    bool stop;

public:
    explicit ThreadPool(const int max_threads) : stop(false) {
        pthread_mutex_init(&queueMutex, nullptr);
        pthread_cond_init(&condition, nullptr);

        for (int i = 0; i < max_threads; ++i) {
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

        for (const pthread_t& thread : threads) {
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
        auto* pool = static_cast<ThreadPool*>(arg);
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


#endif //THREADPOOL_H

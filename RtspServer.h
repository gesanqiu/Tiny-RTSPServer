//
// Created by ricardo on 4/2/23.
//

#ifndef RTSPSERVER_H
#define RTSPSERVER_H


#include "ConnectionHandler.h"
#include "ThreadPool.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

class RTSPServer {
public:
    explicit RTSPServer(int port, size_t num_threads = 4);
    ~RTSPServer();

    void initialize();
    void run();
    void stop();

private:
    void handle_client(int client_fd);
    void remove_connection(int client_fd);

    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    std::mutex connections_mutex_;
    std::map<int, std::shared_ptr<ConnectionHandler>> connections_;
    ThreadPool thread_pool_;
};


#endif //RTSPSERVER_H

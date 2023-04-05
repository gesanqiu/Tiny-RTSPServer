//
// Created by ricardo on 4/2/23.
//

#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "RtspServer.h"
#include "Logger.h"

RTSPServer::RTSPServer(int port, size_t num_threads) : port_(port), server_fd_(-1), running_(false), thread_pool_(num_threads) {}

RTSPServer::~RTSPServer() {
    stop();
    if (server_fd_ >= 0) {
        close(server_fd_);
    }
}

void RTSPServer::initialize() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Error creating socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Error setting socket options");
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Error binding socket to address");
    }

    if (listen(server_fd_, 5) < 0) {
        throw std::runtime_error("Error listening on socket");
    }

    LOG_INFO("RTSP Server initialized on port: " + std::to_string(port_));
}

void RTSPServer::run() {
    LOG_INFO("RTSP Server started");
    running_ = true;

    while (running_) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            LOG_ERROR("Error accepting client connection");
            continue;
        }

        std::string client_ip = std::string(inet_ntoa(client_addr.sin_addr));
        LOG_INFO("Client connected from: {}", client_ip);

        // Create a new ConnectionHandler for each client connection
        std::thread client_thread(&RTSPServer::handle_client, this, client_fd, client_ip);
        client_thread.detach();
    }
}

void RTSPServer::handle_client(int client_fd, const std::string& client_ip) {
    auto handler = std::make_shared<ConnectionHandler>(client_fd, client_ip);
    {
        std::unique_lock<std::mutex> lock(connections_mutex_);
        connections_[client_fd] = handler;
    }

    // Enqueue the task to the ThreadPool
    thread_pool_.enqueue([this, handler, client_fd] {
        handler->process();
        remove_connection(client_fd);
        close(client_fd);
    });
}

void RTSPServer::stop() {
    running_ = false;
}

void RTSPServer::remove_connection(int client_fd) {
    std::unique_lock<std::mutex> lock(connections_mutex_);
    connections_.erase(client_fd);
}

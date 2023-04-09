//
// Created by ricardo on 4/2/23.
//

#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "RtspMessage.h"
#include "PortPool.h"
#include <string>

class ConnectionHandler {
public:
    explicit ConnectionHandler(int client_socket, const std::string& client_ip, PortPool& port_pool);
    ~ConnectionHandler();

    void process();

private:
    std::string receive_rtsp_message();
    void send_rtsp_message(const std::string& message);

    void handle_options(const RTSPMessage& request, RTSPMessage& response);
    void handle_describe(const RTSPMessage& request, RTSPMessage& response);
    void handle_setup(const RTSPMessage& request, RTSPMessage& response);
    void handle_play(const RTSPMessage& request, RTSPMessage& response);
    void handle_pause(const RTSPMessage& request, RTSPMessage& response);
    void handle_teardown(const RTSPMessage& request, RTSPMessage& response);

    int client_socket_;
    std::string  client_ip_;
    PortPool& port_pool_;
    std::string session_id_;
    std::string generate_unique_session_id();
};

#endif // CONNECTION_HANDLER_H

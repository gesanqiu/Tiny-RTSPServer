//
// Created by ricardo on 4/2/23.
//

#include "ConnectionHandler.h"

#include "Logger.h"
#include <sys/socket.h>
#include <unistd.h>
#include <regex>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

ConnectionHandler::ConnectionHandler(int client_socket)
        : client_socket_(client_socket) {
}

ConnectionHandler::~ConnectionHandler() {
    close(client_socket_);
}

void ConnectionHandler::process() {
    try {
        while (true) {
            // Receive and parse the RTSP message
            RTSPMessage request;
            std::string received_data = receive_rtsp_message();
            if (!request.parse(received_data)) {
                LOG_ERROR("Failed to parse RTSP message");
                return;
            }

            // Handle the request and generate a response
            RTSPMessage response;
            response.set_cseq(request.cseq());
            std::map<std::string, std::string> response_headers;

            switch (request.method()) {
                case RTSPMessage::Method::OPTIONS:
                    handle_options(request, response);
                    break;
                case RTSPMessage::Method::DESCRIBE:
                    handle_describe(request, response);
                    break;
                case RTSPMessage::Method::SETUP:
                    handle_setup(request, response);
                    break;
                case RTSPMessage::Method::PLAY:
                    handle_play(request, response);
                    break;
                case RTSPMessage::Method::PAUSE:
                    handle_pause(request, response);
                    break;
                case RTSPMessage::Method::TEARDOWN:
                    handle_teardown(request, response);
                    break;
                default:
                    response.set_status_code(RTSPMessage::StatusCode::MethodNotAllowed);
                    break;
            }

            // Send the response
            std::string response_str = response.create_response();
            send_rtsp_message(response_str);

            // Close the connection if it's a TEARDOWN request
            if (request.method() == RTSPMessage::Method::TEARDOWN) {
                break;
            } else if (request.method() == RTSPMessage::Method::PLAY) {
                // Only use for RTSPServer unit test
                while (true) {
                    usleep(40 * 1000);
                }
            }
        }
    } catch (const std::runtime_error& e) {
        LOG_ERROR("Connection error: {}", e.what());
    } catch (...) {
        LOG_ERROR("An unknown error occurred during client connection handling.");
    }

    LOG_INFO("Client connection closed.");
}

std::string ConnectionHandler::receive_rtsp_message() {
    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    ssize_t received_bytes = recv(client_socket_, buffer, BUFFER_SIZE - 1, 0);

    if (received_bytes <= 0) {
        if (received_bytes == 0) {
            LOG_INFO("Client closed the connection");
        } else {
            LOG_ERROR("Error while receiving data from client");
        }
        throw std::runtime_error("Failed to receive RTSP message");
    }

    buffer[received_bytes] = '\0';
    return std::string(buffer);
}

void ConnectionHandler::send_rtsp_message(const std::string& message) {
    ssize_t bytes_sent = send(client_socket_, message.c_str(), message.size(), 0);

    if (bytes_sent < 0) {
        LOG_ERROR("Failed to send RTSP message to client");
        throw std::runtime_error("Failed to send RTSP message");
    } else if (bytes_sent != static_cast<ssize_t>(message.size())) {
        LOG_ERROR("Incomplete RTSP message sent to client");
        throw std::runtime_error("Incomplete RTSP message sent");
    }
}

std::string ConnectionHandler::generate_unique_session_id() {
    // Generate a random number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);

    // Get the current timestamp
    auto now = std::chrono::system_clock::now();
    auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Combine the random number and timestamp to create a unique session ID
    std::stringstream ss;
    ss << std::setw(10) << std::setfill('0') << seconds_since_epoch << std::setw(6) << std::setfill('0') << dis(gen);
    return ss.str();
}

void ConnectionHandler::handle_options(const RTSPMessage& request, RTSPMessage& response) {
    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Public", "OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN");
}

void ConnectionHandler::handle_describe(const RTSPMessage& request, RTSPMessage& response) {
    // Check the Accept header and make sure the client accepts SDP format
    if (request.get_header_value("Accept") != "application/sdp") {
        response.set_status_code(RTSPMessage::StatusCode::NotAcceptable);
        response.set_cseq(request.cseq());
        return;
    }

    // 最终实现时根据request_uri_借助ResourceManager完成鉴权和查找流的工作，sdp也由RTSPStream对象自行维护
    // Create a media description (SDP) for the requested media resource
    auto request_uri = request.uri();
    std::string username, password, host, port;
    std::regex rtsp_url_regex("rtsp://(?:([^:]+):([^@]+)@)?((?:[0-9a-zA-Z.-]+)|(?:\\d{1,3}(?:\\.\\d{1,3}){3}))(?::(\\d+))?.*");
    std::smatch url_match;
    if (std::regex_search(request_uri, url_match, rtsp_url_regex)) {
        if (url_match.size() > 4) {
            username = url_match[1].str();
            password = url_match[2].str();
            host = url_match[3].str();
            port = url_match[4].str();
        }
    }
    LOG_INFO("username: {}, password: {}, host:{}, port: {}", username, password, host, port);

    std::ostringstream  oss;
    oss << "v=0\r\n"
           "o=- 666 1 IN IP4 " << host << "\r\n"
           "t=0 0\r\n"
           "a=control:*\r\n"
           "m=video 0 RTP/AVP 96\r\n"
           "a=rtpmap:96 H264/90000\r\n"
           "a=control:track0\r\n";
    auto sdp = oss.str();
    // Set the response attributes
    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Content-Type", "application/sdp");
    response.set_headers("Content-Length", std::to_string(sdp.size()));
    response.set_body(sdp);
}

void ConnectionHandler::handle_setup(const RTSPMessage& request, RTSPMessage& response) {
    // Parse transport and client ports from the request
    auto transport = request.get_header_value("Transport");
    std::string client_port_range;

    std::regex transport_regex("RTP/AVP;unicast;client_port=(\\d+)-(\\d+)");
    std::smatch transport_match;
    if (std::regex_search(transport, transport_match, transport_regex)) {
        if (transport_match.size() > 2) {
            client_port_range = transport_match[1].str() + "-" + transport_match[2].str();
        }
    }

    // Generate server ports and SSRC
    std::string server_port_range = "9000-9001"; // You can generate server ports dynamically
    std::string ssrc = "1A2B3C4D"; // You can generate an SSRC dynamically

    // Generate a session ID
    std::string session_id = generate_unique_session_id();
//    session_manager_.create_session(session_id);
    LOG_INFO("Session id: {}", session_id);
    // Build the response
    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Transport", "RTP/AVP;unicast;client_port=" + client_port_range + ";server_port=" + server_port_range + ";ssrc=" + ssrc);
    response.set_headers("Session", session_id);
}

void ConnectionHandler::handle_play(const RTSPMessage& request, RTSPMessage& response) {
    std::string session_id = request.get_header_value("Session");
    LOG_INFO("Play session: {}", session_id);

    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Session", session_id);
    std::string range = request.get_header_value("Range");
    response.set_headers("Range", range);
    std::string rtp_info = "url=" + request.uri() + ";seq=1;rtptime=0";
    response.set_headers("RTP-Info", rtp_info);
}

void ConnectionHandler::handle_pause(const RTSPMessage& request, RTSPMessage& response) {
    std::string session_id = request.get_header_value("Session");
    LOG_INFO("Pause session: {}", session_id);
    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Session", session_id);
}

void ConnectionHandler::handle_teardown(const RTSPMessage& request, RTSPMessage& response) {
    std::string session_id = request.get_header_value("Session");
    LOG_INFO("Teardown session: {}", session_id);
    response.set_protocol(request.protocol());
    response.set_status_code(RTSPMessage::StatusCode::OK);
    response.set_cseq(request.cseq());
    response.set_headers("Session", session_id);
}
//
// Created by ricardo on 4/2/23.
//

#include "Session.h"
#include <unistd.h>

Session::Session(const std::string& session_id, const std::string& media_url,
                 const int server_rtp_port, const int server_rtcp_port, const std::string& client_ip,
                 const int client_rtp_port, const int client_rtcp_port) : session_id_(session_id) {
    media_source_ = std::make_unique<H264MediaSource>(media_url);
    rtp_rtcp_handler_ = std::make_unique<RtpRtcpHandler>(server_rtp_port, server_rtcp_port,
                                                         client_ip, client_rtp_port, client_rtcp_port,
                                                         media_source_->get_timestamp_increment());
}

Session::~Session() {
    // Clean up resources, such as stopping the stream if still active
    stop();
}

const std::string& Session::session_id() const {
    return session_id_;
}

bool Session::start() {
    // Start the media stream
    // Implement your actual media stream start logic here
    return true; // Return true if successful, false otherwise
}

bool Session::pause() {
    // Pause the media stream
    // Implement your actual media stream pause logic here
    return true; // Return true if successful, false otherwise
}

bool Session::stop() {
    // Stop the media stream
    // Implement your actual media stream stop logic here
    return true; // Return true if successful, false otherwise
}


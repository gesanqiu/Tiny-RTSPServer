//
// Created by ricardo on 4/2/23.
//

#include "Session.h"
#include <unistd.h>
#include <thread>
#include "Logger.h"

Session::Session(const std::string& session_id, const std::string& media_url,
                 const int server_rtp_port, const int server_rtcp_port, const std::string& client_ip,
                 const int client_rtp_port, const int client_rtcp_port) : session_id_(session_id) {
    media_source_ = std::make_unique<H264MediaSource>("/home/ricardo/Downloads/fire.h264");
    rtp_rtcp_handler_ = std::make_unique<RtpRtcpHandler>(server_rtp_port, server_rtcp_port,
                                                         client_ip, client_rtp_port, client_rtcp_port,
                                                         3600);
    media_source_->open();
}

Session::~Session() {
    // Clean up resources, such as stopping the stream if still active
    stop();
}

const std::string& Session::session_id() const {
    return session_id_;
}

bool Session::start() {
    if (!is_active_) {
        is_active_ = true;
        // Start sending media data
        std::thread sender_thread([this]() {
            while (is_active_) {

                size_t frame_size = media_source_->get_next_frame();
                if (frame_size <= 0) {
                    LOG_INFO("Read to the end, exit.");
                    break;
                }

                rtp_rtcp_handler_->send_rtp_packet(media_source_->get_frame_buf(), frame_size);

                // Sleep for a duration based on the frame rate
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
        });

        sender_thread.detach();
    }
    return true; // Return true if successful, false otherwise
}

bool Session::pause() {
    is_active_ = false;
    return true; // Return true if successful, false otherwise
}

bool Session::stop() {
    is_active_ = false;
    media_source_->close();
    return true; // Return true if successful, false otherwise
}


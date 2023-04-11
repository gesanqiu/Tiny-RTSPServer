//
// Created by ricardo on 4/2/23.
//

#include "Session.h"
#include <unistd.h>
#include <thread>
#include "Logger.h"

Session::Session(const std::string& session_id, const std::shared_ptr<MediaSource>& media_source,
                 const int server_rtp_port, const int server_rtcp_port, const std::string& client_ip,
                 const int client_rtp_port, const int client_rtcp_port) : session_id_(session_id) {
    media_source_ = media_source;
    rtp_rtcp_handler_ = std::make_shared<RtpRtcpHandler>(server_rtp_port, server_rtcp_port,
                                                         client_ip, client_rtp_port, client_rtcp_port,
                                                         media_source_->get_timestamp_increment());
    media_source_->open();
}

Session::~Session() {
    // Clean up resources, such as stopping the stream if still active
    stop();
    if (media_source_) media_source_.reset();
}

const std::string& Session::session_id() const {
    return session_id_;
}

bool Session::start() {
    if (!is_active_) {
        is_active_ = true;
        // Start sending media data
        sender_thread_.reset(new std::thread([this]() {
            while (is_active_) {
                if (media_source_) {
                    int frame_size = media_source_->get_next_frame();
                    if (frame_size <= 0) {
                        LOG_INFO("Read to the end, exit.");
                        break;
                    }

                    rtp_rtcp_handler_->send_rtp_packet(media_source_->get_frame_buf(), frame_size, media_source_->get_media_type());

                    // Sleep for a duration based on the frame rate
                    std::this_thread::sleep_for(std::chrono::microseconds(1000000 / media_source_->get_media_fps()));
                } else {
                    throw std::runtime_error("media source is null.");
                }
            }
            LOG_INFO("Session thread stopped.");
        }));
    }
    return true; // Return true if successful, false otherwise
}

bool Session::pause() {
    is_active_ = false;
    return true; // Return true if successful, false otherwise
}

bool Session::stop() {
    LOG_INFO("session stop: {}", session_id_);
    is_active_ = false;
    if (sender_thread_ && sender_thread_->joinable()) {
        sender_thread_->join();
        sender_thread_.reset();
    }
    if (media_source_) media_source_->close();
    LOG_INFO("session stopped: {}", session_id_);
    return true; // Return true if successful, false otherwise
}

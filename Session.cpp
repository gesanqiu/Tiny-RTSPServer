//
// Created by ricardo on 4/2/23.
//

#include "Session.h"
#include <unistd.h>
#include <thread>
#include "Logger.h"

Session::Session(const std::string& session_id) : session_id_(session_id) {}

Session::~Session() {
    // Clean up resources, such as stopping the stream if still active
    stop();
    for (auto& [k, v] : session_handlers_) {
        if (v) {
            if (v->media_track_) {
                v->media_track_->unregister_session(session_id_);
                v->media_track_.reset();
            }
            if (v->rtp_rtcp_handler_) v->rtp_rtcp_handler_.reset();
            v.reset();
        }
    }
    session_handlers_.clear();
}

const std::string& Session::session_id() const {
    return session_id_;
}

void Session::add_media_track(const std::string& track_name, const std::shared_ptr<MediaTrack>& media_track,
    const int server_rtp_port, const int server_rtcp_port, const std::string& client_ip,
    const int client_rtp_port, const int client_rtcp_port) {
    auto session_handler = std::make_shared<SessionHandler>();
    session_handler->track_name_ = track_name;
    session_handler->media_track_ = media_track;
    session_handler->media_track_->register_session(session_id_);
    auto rtp_rtcp_handler = std::make_shared<RtpRtcpHandler>(server_rtp_port, server_rtcp_port,
                                                         client_ip, client_rtp_port, client_rtcp_port,
                                                         media_track->get_timestamp_increment());
    session_handler->rtp_rtcp_handler_ = rtp_rtcp_handler;
    session_handlers_[track_name] = session_handler;
}

bool Session::start() {
    for (auto& [k, v] : session_handlers_) {
        // Start sending media data
        if (!v->is_active_) {
            LOG_INFO("Track: {} started.", k);
            v->is_active_  = true;
            v->media_track_->start();   // 因为是从文件里读，所以需要手动start和stop，正常处理一个流的时候应该假设始终能product
            v->sender_thread_.reset(new std::thread([this, &v]() {
                while (v->is_active_) {
                    if (v->media_track_) {
                        auto frame = v->media_track_->get_next_frame(this->session_id_);
                        if (nullptr == frame) {
                            LOG_INFO("Read to the end, exit.");
                            break;
                        }

                        v->rtp_rtcp_handler_->send_rtp_packet(frame->data_, frame->size_, v->media_track_->get_media_type());

                        // Sleep for a duration based on the frame rate
                        std::this_thread::sleep_for(std::chrono::microseconds(1000000 / v->media_track_->get_media_fps()));
                    } else {
                        throw std::runtime_error("media source is null.");
                    }
                }
                LOG_INFO("Session thread stopped.");
            }));
        }
    }
    return true; // Return true if successful, false otherwise
}

bool Session::pause() {
    for (auto& [k, v] : session_handlers_) {
        if (v) {
            v->is_active_ = false;
        }
    }
    return true; // Return true if successful, false otherwise
}

bool Session::stop() {
    LOG_INFO("session stop: {}", session_id_);
    for (auto& [k, v] : session_handlers_) {
        if (v) {
            v->is_active_ = false;
            if (v->sender_thread_ && v->sender_thread_->joinable()) {
                v->sender_thread_->join();
                v->sender_thread_.reset();
            }
//            if (v->media_track_) v->media_track_->stop();
        }
    }
    LOG_INFO("session stopped: {}", session_id_);
    return true; // Return true if successful, false otherwise
}

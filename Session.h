//
// Created by ricardo on 4/2/23.
//

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <memory>
#include <thread>
#include <unordered_map>

#include "MediaSourceHandler.h"
#include "RtpRtcpHandler.h"

struct SessionHandler {
    bool is_active_;
    std::string track_name_;
    std::shared_ptr<std::thread> sender_thread_;
    std::shared_ptr<MediaTrack> media_track_;
    std::shared_ptr<RtpRtcpHandler> rtp_rtcp_handler_;
};

class Session {
public:
    Session(const std::string& session_id);

    ~Session();

    const std::string& session_id() const;
    void add_media_track(const std::string& track_name, const std::shared_ptr<MediaTrack>& media_track, const int server_rtp_port,
            const int server_rtcp_port, const std::string &client_ip, const int client_rtp_port, const int client_rtcp_port);

    // Methods for controlling the media stream
    bool start();
    bool pause();
    bool stop();

private:
    std::string session_id_;
    std::unordered_map<std::string, std::shared_ptr<SessionHandler>> session_handlers_;
};

#endif //SESSION_H

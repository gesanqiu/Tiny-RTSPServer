//
// Created by ricardo on 4/2/23.
//

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <memory>
#include <thread>

#include "MediaSourceHandler.h"
#include "RtpRtcpHandler.h"

class Session {
public:
    Session(const std::string &session_id, const std::string &media_url, const int server_rtp_port,
            const int server_rtcp_port, const std::string &client_ip, const int client_rtp_port,
            const int client_rtcp_port);

    ~Session();

    const std::string& session_id() const;

    // Methods for controlling the media stream
    bool start();
    bool pause();
    bool stop();

private:
    std::string session_id_;
    bool is_active_;
    std::shared_ptr<std::thread> sender_thread_;
    std::unique_ptr<MediaSource> media_source_;
    std::unique_ptr<RtpRtcpHandler> rtp_rtcp_handler_;
};

#endif //SESSION_H

//
// Created by ricardo on 4/2/23.
//

#include "SessionManager.h"

std::shared_ptr<Session> SessionManager::create_session(const std::string& session_id, const std::string& media_url,
                                                        const int server_rtp_port, const int server_rtcp_port,
                                                        const std::string& client_ip, const int client_rtp_port,
                                                        const int client_rtcp_port) {
    std::unique_lock<std::mutex> lock(sessions_mutex_);
    auto session = std::make_shared<Session>(session_id, media_url, server_rtcp_port, server_rtp_port, client_ip, client_rtp_port, client_rtcp_port);
    sessions_[session_id] = session;
    return session;
}

std::shared_ptr<Session> SessionManager::get_session(const std::string& session_id) {
    std::unique_lock<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

bool SessionManager::session_exists(const std::string& session_id) {
    std::unique_lock<std::mutex> lock(sessions_mutex_);
    return sessions_.find(session_id) != sessions_.end();
}

void SessionManager::terminate_session(const std::string& session_id) {
    std::unique_lock<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second->stop(); // Stop the session before erasing it
        sessions_.erase(it);
    }
}

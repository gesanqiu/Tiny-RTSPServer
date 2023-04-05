//
// Created by ricardo on 4/2/23.
//

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <memory>
#include <unordered_map>
#include <mutex>

#include "Session.h"

class SessionManager {
public:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    static SessionManager& instance() {
        static SessionManager instance; // This is the single instance
        return instance;
    }

    std::shared_ptr<Session> create_session(const std::string& session_id,
                                            const std::string& media_url,
                                            const int server_rtp_port,
                                            const int server_rtcp_port,
                                            const std::string& client_ip,
                                            const int client_rtp_port,
                                            const int client_rtcp_port);
    std::shared_ptr<Session> get_session(const std::string& session_id);
    void terminate_session(const std::string& session_id);
    bool session_exists(const std::string& session_id);

private:
    SessionManager() {} // Make the constructor private

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::mutex sessions_mutex_;
};

#endif //SESSIONMANAGER_H

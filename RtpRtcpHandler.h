//
// Created by ricardo on 4/2/23.
//

#ifndef RTP_RTCP_HANDLER_H
#define RTP_RTCP_HANDLER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include "MediaSourceHandler.h"

const size_t MAX_RTP_PACKET_SIZE = 1024;

struct RTPHeader {
    // RTP header fields
    uint8_t csrc_count_ : 4;
    uint8_t padding_ : 1;
    uint8_t extension_ : 1;
    uint8_t version_ : 2;
    uint8_t payload_type_ : 7;
    uint8_t marker_ : 1;
    uint16_t sequence_number_;
    uint32_t timestamp_;
    uint32_t ssrc_;
//    uint32_t* csrc_list_;
};

struct RTCPHeader {
    uint8_t report_count_ : 5;
    uint8_t padding_ : 1;
    uint8_t version_ : 2;
    uint8_t packet_type_;
    uint16_t length_;
};

struct RTCPReportBlock {
    uint32_t ssrc_;
    uint8_t fraction_lost_;
    uint32_t cumulative_packets_lost_ : 24;
    uint32_t extended_highest_sequence_number_;
    uint32_t interarrival_jitter_;
    uint32_t last_sr_timestamp_;
    uint32_t delay_since_last_sr_;
};

struct RTCPReceiverReport : RTCPHeader {
    uint32_t ssrc_;
};

struct RTCPSenderReport : RTCPHeader {
    uint32_t ssrc_;
    uint32_t ntp_timestamp_msw_; // Most Significant Word of NTP timestamp
    uint32_t ntp_timestamp_lsw_; // Least Significant Word of NTP timestamp
    uint32_t rtp_timestamp_;
    uint32_t sender_packet_count_;
    uint32_t sender_octet_count_;
};

class RtpRtcpHandler {
public:
    RtpRtcpHandler(const int server_rtp_port,
                   const int server_rtcp_port,
                   const std::string& ip,
                   int rtp_port,
                   int rtcp_port,
                   uint32_t timestamp_increment);
    ~RtpRtcpHandler();

    void send_rtp_packet(const char* payload, size_t payload_size, MediaType payload_type);

    void stop();
    void start();
private:
    int rtp_socket_;
    int rtcp_socket_;
    std::string ip_;
    int rtp_port_;
    int rtcp_port_;
    struct sockaddr_in client_addr_;
    uint32_t ssrc_;
    int rtp_sequence_number_;
    uint32_t timestamp_;
    uint32_t timestamp_increment_;

    char rtp_buf_[MAX_RTP_PACKET_SIZE + 10];

    void send_h264(const char *payload, size_t payload_size);
    void send_aac(const char *payload, size_t payload_size);

    bool is_running_;
    std::shared_ptr<std::thread> rtcp_send_thread_;
    std::shared_ptr<std::thread> rtcp_receive_thread_;
    void send_rtcp_packet();
    void receive_rtcp_packet();
    std::mutex rtcp_send_receive_mutex_;
    std::unordered_map<uint32_t, RTCPReportBlock> rtcp_report_blocks_;
    uint32_t total_payload_size_;
    uint32_t total_rtp_packet_sent_;
    bool parse_rtcp_packet(const char *buffer, ssize_t received_bytes, RTCPReceiverReport &rr);
};

#endif // RTP_RTCP_HANDLER_H

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

const size_t MAX_RTP_PACKET_SIZE = 512 * 1024;

struct RTPHeader {
    enum PayloadType : uint8_t {
        H264 = 96,
        AAC = 97
    };
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


class RtpRtcpHandler {
public:
    RtpRtcpHandler(const int server_rtp_port,
                   const int server_rtcp_port,
                   const std::string& ip,
                   int rtp_port,
                   int rtcp_port,
                   uint32_t timestamp_increment);
    ~RtpRtcpHandler();

    void send_rtp_packet(const char* payload, size_t payload_size);
    void receive_rtcp_packet();

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

    void send_rtcp_packet();
    char rtp_buf_[MAX_RTP_PACKET_SIZE + 10];
};

#endif // RTP_RTCP_HANDLER_H

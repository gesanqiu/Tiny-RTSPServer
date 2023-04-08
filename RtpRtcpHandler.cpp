//
// Created by ricardo on 4/2/23.
//

#include "RtpRtcpHandler.h"
#include "Logger.h"
#include <random>
#include <cstring>

RtpRtcpHandler::RtpRtcpHandler(const int server_rtp_port, const int server_rtcp_port,
                               const std::string& ip, int rtp_port, int rtcp_port, uint32_t timestamp_increment)
       : ip_(ip), rtp_port_(rtp_port), rtcp_port_(rtcp_port), rtp_sequence_number_(0),
       timestamp_(0), timestamp_increment_(timestamp_increment) {
    // Generate a random SSRC
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);
    ssrc_ = dist(mt);

    rtp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (rtp_socket_ < 0) {
        throw std::runtime_error("RtpRtcpHandler create server socket failed");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_rtp_port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(rtp_socket_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0) {
        throw std::runtime_error("Bind rtp socket with server failed.");
    }
}

RtpRtcpHandler::~RtpRtcpHandler() {
    close(rtp_socket_);
}

void RtpRtcpHandler::send_rtp_packet(const char* payload, size_t payload_size) {
    constexpr size_t max_payload_size = MAX_RTP_PACKET_SIZE - sizeof(RTPHeader);
    size_t start_code, nal_unit_size;
    if (payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x01) {
        nal_unit_size = payload_size - 3;
        start_code = 3;
    } else if (payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x00 && payload[3] == 0x01) {
        nal_unit_size = payload_size - 4;
        start_code = 4;
    } else {
        LOG_ERROR("Error format H.264 frame, start code fault.");
        return ;
    }

    // Check if the payload size exceeds the max_payload_size
    if (nal_unit_size > max_payload_size) {
        // Fragment the NAL unit
        size_t offset = start_code + 1; // Skip the start code and NALU type byte
        bool first_fragment = true;
        ssize_t send_bytes = 0;
        while (offset < payload_size) {
            // Calculate the size of the current fragment
            size_t fragment_size = std::min(max_payload_size - 2, payload_size - offset);   // 2 bytes for FU Indicator and FU Header

            // Create the FU header
            uint8_t fu_header = (first_fragment ? 0x80 : 0x00) | (offset + fragment_size == payload_size ? 0x40 : 0x00) | (payload[start_code] & 0x1F);

            // Create the RTP packet with the FU header
            RTPHeader rtp_header;
            rtp_header.version_ = 2;
            rtp_header.padding_ = 0;
            rtp_header.extension_ = 0;
            rtp_header.csrc_count_ = 0;
            rtp_header.marker_ = offset + fragment_size == payload_size ? 1 : 0;
            rtp_header.payload_type_ = RTPHeader::PayloadType::H264;
            rtp_header.sequence_number_ = htons(rtp_sequence_number_++);
            rtp_header.timestamp_ = htonl(timestamp_);
            rtp_header.ssrc_ = htonl(ssrc_);

            size_t header_size = sizeof(RTPHeader);
            memcpy(rtp_buf_, &rtp_header, header_size);
            rtp_buf_[header_size] = (payload[start_code] & 0x60) | 28;   // FU indicator (F, NRI, and type)
            rtp_buf_[header_size + 1] = fu_header;   // FU header (S, E, R, and type)
            memcpy(rtp_buf_ + header_size + 2, payload + offset, fragment_size);

            // Send the RTP packet through the connected UDP socket
            memset(&client_addr_, 0, sizeof(client_addr_));
            client_addr_.sin_family = AF_INET;
            client_addr_.sin_port = htons(rtp_port_);
            if (inet_pton(AF_INET, ip_.c_str(), &client_addr_.sin_addr) <= 0) {
                throw std::runtime_error("RtpRtcpHandler create client udp socket failed");
            }
            size_t rtp_packet_size = header_size + 2 + fragment_size;
            ssize_t ret = sendto(rtp_socket_, rtp_buf_, rtp_packet_size, 0,
                                 (struct sockaddr *)&client_addr_, sizeof(client_addr_));
            send_bytes += ret;
            // Update the offset and first_fragment flag
            offset += fragment_size;
            first_fragment = false;
        }
        timestamp_ += timestamp_increment_;
    } else {
        // Create the RTP packet without fragmentation
        RTPHeader rtp_header;
        rtp_header.version_ = 2;
        rtp_header.padding_ = 0;
        rtp_header.extension_ = 0;
        rtp_header.csrc_count_ = 0;
        rtp_header.marker_ = 1;
        rtp_header.payload_type_ = RTPHeader::PayloadType::H264;
        rtp_header.sequence_number_ = htons(rtp_sequence_number_++);
        rtp_header.timestamp_ = htonl(timestamp_);
        rtp_header.ssrc_ = htonl(ssrc_);

        size_t header_size = sizeof(RTPHeader);
        memcpy(rtp_buf_, &rtp_header, header_size);     // copy header data
        memcpy(rtp_buf_ + header_size, payload + start_code, payload_size - start_code);

        // Send the RTP packet
        memset(&client_addr_, 0, sizeof(client_addr_));
        client_addr_.sin_family = AF_INET;
        client_addr_.sin_port = htons(rtp_port_);
        if (inet_pton(AF_INET, ip_.c_str(), &client_addr_.sin_addr) <= 0) {
            throw std::runtime_error("RtpRtcpHandler create client udp socket failed");
        }
        size_t rtp_packet_size = header_size + payload_size - start_code;
        ssize_t  ret = sendto(rtp_socket_, rtp_buf_, rtp_packet_size, 0,
                              (struct sockaddr *)&client_addr_, sizeof(client_addr_));
        if ((payload[start_code] & 0x1F) != 7 && (payload[start_code] & 0x1F) != 8) {   // Skip SPS & PPS
            timestamp_ += timestamp_increment_;
        }
    }

}

void RtpRtcpHandler::receive_rtcp_packet() {
    // Receive and process RTCP packets
}

void RtpRtcpHandler::send_rtcp_packet() {
    // Send RTCP packets
}
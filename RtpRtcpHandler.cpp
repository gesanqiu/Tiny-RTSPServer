//
// Created by ricardo on 4/2/23.
//

#include "RtpRtcpHandler.h"
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

    server_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("RtpRtcpHandler create server socket failed");
    }

    memset(&client_addr_, 0, sizeof(client_addr_));
    client_addr_.sin_family = AF_INET;
    client_addr_.sin_port = htons(rtp_port);
    if (inet_pton(AF_INET, ip.c_str(), &client_addr_.sin_addr) <= 0) {
        // Handle the error (e.g., throw an exception or return a status code)
        throw std::runtime_error("RtpRtcpHandler create client udp socket failed");
    }
}

RtpRtcpHandler::~RtpRtcpHandler() {
}

void RtpRtcpHandler::send_rtp_packet(const uint8_t* payload, size_t payload_size) {
    constexpr size_t max_payload_size = MAX_RTP_PACKET_SIZE - sizeof(RTPHeader);
    size_t remaining_payload_size = payload_size;
    const uint8_t* current_payload_position = payload;
    RTPHeader rtp_packet;
    rtp_packet.version_ = 2;
    rtp_packet.padding_ = 0;
    rtp_packet.extension_ = 0;
    rtp_packet.csrc_count_ = 0;
    rtp_packet.payload_type_ = RTPHeader::PayloadType::H264;
    rtp_packet.timestamp_ = htonl(timestamp_);
    rtp_packet.ssrc_ = htonl(ssrc_);

    while (remaining_payload_size > 0) {
        // Calculate the size of the payload for this packet
        size_t current_payload_size = std::min(remaining_payload_size, max_payload_size);

        rtp_packet.marker_ = (remaining_payload_size == current_payload_size) ? 1 : 0;
        rtp_packet.sequence_number_ = htons(rtp_sequence_number_++);

        // Allocate a buffer for the RTP packet with the header and payload
        size_t rtp_packet_size = sizeof(RTPHeader) + current_payload_size;

        // Copy the RTP header to the buffer
        memcpy(rtp_buf_, &rtp_packet, sizeof(RTPHeader));

        // Copy the payload data to the buffer, right after the RTP header
        memcpy(rtp_buf_ + sizeof(RTPHeader), current_payload_position, current_payload_size);

        // Send the RTP packet through the connected UDP socket
        ssize_t bytes_sent = sendto(server_fd_, rtp_buf_, rtp_packet_size, 0,
                                    (struct sockaddr *) &client_addr_, sizeof(client_addr_));

        // Check if the packet was sent successfully
        if (bytes_sent != rtp_packet_size) {
            // Handle the error (e.g., log the error, throw an exception, or return a status code)
        }

        current_payload_position += current_payload_size;
        remaining_payload_size -= current_payload_size;
    }
    timestamp_ += timestamp_increment_;
}

void RtpRtcpHandler::receive_rtcp_packet() {
    // Receive and process RTCP packets
}

void RtpRtcpHandler::send_rtcp_packet() {
    // Send RTCP packets
}
//
// Created by ricardo on 4/2/23.
//

#include "RtpRtcpHandler.h"
#include "Logger.h"
#include <random>
#include <cstring>
#include <sys/time.h>

RtpRtcpHandler::RtpRtcpHandler(const int server_rtp_port, const int server_rtcp_port,
                               const std::string& ip, int rtp_port, int rtcp_port, uint32_t timestamp_increment)
       : ip_(ip), rtp_port_(rtp_port), rtcp_port_(rtcp_port), rtp_sequence_number_(0),
       timestamp_(0), timestamp_increment_(timestamp_increment), is_running_(true),
       total_payload_size_(0), total_rtp_packet_sent_(0) {
    // Generate a random SSRC
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);
    ssrc_ = dist(mt);

    rtp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (rtp_socket_ < 0) {
        throw std::runtime_error("RtpRtcpHandler create server rtp socket failed");
    }

    int opt = 1;
    if (setsockopt(rtp_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Error setting rtp_socket_ options");
    }

    struct sockaddr_in rtp_addr;
    rtp_addr.sin_family = AF_INET;
    rtp_addr.sin_port = htons(server_rtp_port);
    rtp_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(rtp_socket_, (struct sockaddr*)&rtp_addr, sizeof(struct sockaddr)) < 0) {
        throw std::runtime_error("Bind rtp socket with server failed.");
    }

    rtcp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (rtcp_socket_ < 0) {
        throw std::runtime_error("RtpRtcpHandler create server rtcp socket failed");
    }

    opt = 1;
    if (setsockopt(rtcp_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Error setting rtp_socket_ options");
    }

    struct timeval tv;
    tv.tv_sec  = 1;     // 1s超时
    tv.tv_usec = 0;
    if (setsockopt(rtcp_socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0) {
        throw std::runtime_error("Error setting rtcp_socket_ timeout options");
    }

    struct sockaddr_in rtcp_addr;
    rtcp_addr.sin_family = AF_INET;
    rtcp_addr.sin_port = htons(server_rtcp_port);
    rtcp_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(rtcp_socket_, (struct sockaddr*)&rtcp_addr, sizeof(struct sockaddr)) < 0) {
        throw std::runtime_error("Bind rtcp socket with server failed.");
    }
}

RtpRtcpHandler::~RtpRtcpHandler() {
    stop();

    if (rtp_socket_ >= 0) close(rtp_socket_);
    if (rtcp_socket_ >= 0) close(rtcp_socket_);
}

void RtpRtcpHandler::start() {
    rtcp_send_thread_.reset(new std::thread(&RtpRtcpHandler::send_rtcp_packet, this));
    rtcp_receive_thread_.reset(new std::thread(&RtpRtcpHandler::receive_rtcp_packet, this));
}

void RtpRtcpHandler::stop() {
    is_running_ = false;
    if (rtcp_send_thread_ && rtcp_send_thread_->joinable()) {
        rtcp_send_thread_->join();
        rtcp_send_thread_.reset();
    }
    if (rtcp_receive_thread_ && rtcp_receive_thread_->joinable()) {
        rtcp_receive_thread_->join();
        rtcp_receive_thread_.reset();
    }
}

void RtpRtcpHandler::send_h264(const char* payload, size_t payload_size) {
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
            rtp_header.payload_type_ = static_cast<uint8_t>(MediaType::H264);
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
                throw std::runtime_error("RtpRtcpHandler bind client rtp socket address failed");
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
        ++total_rtp_packet_sent_;
    } else {
        // Create the RTP packet without fragmentation
        RTPHeader rtp_header;
        rtp_header.version_ = 2;
        rtp_header.padding_ = 0;
        rtp_header.extension_ = 0;
        rtp_header.csrc_count_ = 0;
        rtp_header.marker_ = 1;
        rtp_header.payload_type_ = static_cast<uint8_t>(MediaType::H264);
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
            throw std::runtime_error("RtpRtcpHandler bind client rtp socket address failed");
        }
        size_t rtp_packet_size = header_size + payload_size - start_code;
        ssize_t ret = sendto(rtp_socket_, rtp_buf_, rtp_packet_size, 0,
                              (struct sockaddr *)&client_addr_, sizeof(client_addr_));
        if ((payload[start_code] & 0x1F) != 7 && (payload[start_code] & 0x1F) != 8) {   // Skip SPS & PPS
            timestamp_ += timestamp_increment_;
        }
        ++total_rtp_packet_sent_;
    }
}

void RtpRtcpHandler::send_aac(const char* payload, size_t payload_size) {
    RTPHeader rtp_header;
    rtp_header.version_ = 2;
    rtp_header.padding_ = 0;
    rtp_header.extension_ = 0;
    rtp_header.csrc_count_ = 0;
    rtp_header.marker_ = 1;
    rtp_header.payload_type_ = static_cast<uint8_t>(MediaType::AAC);
    rtp_header.sequence_number_ = htons(rtp_sequence_number_++);
    rtp_header.timestamp_ = htonl(timestamp_);
    rtp_header.ssrc_ = htonl(ssrc_);

    size_t header_size = sizeof(RTPHeader);
    memcpy(rtp_buf_, &rtp_header, header_size);     // copy header data
    rtp_buf_[header_size] = 0x00;
    rtp_buf_[header_size + 1] = 0x10;
    rtp_buf_[header_size + 2] = (payload_size & 0x1FE0) >> 5;
    rtp_buf_[header_size + 3] = (payload_size & 0x1F) << 3;
    memcpy(rtp_buf_ + header_size + 4, payload, payload_size);

    // Send the RTP packet
    memset(&client_addr_, 0, sizeof(client_addr_));
    client_addr_.sin_family = AF_INET;
    client_addr_.sin_port = htons(rtp_port_);
    if (inet_pton(AF_INET, ip_.c_str(), &client_addr_.sin_addr) <= 0) {
        throw std::runtime_error("RtpRtcpHandler bind client rtp socket address failed");
    }
    size_t rtp_packet_size = header_size + 4 + payload_size;
    ssize_t  ret = sendto(rtp_socket_, rtp_buf_, rtp_packet_size, 0,
                          (struct sockaddr *)&client_addr_, sizeof(client_addr_));
    timestamp_ += timestamp_increment_;
    ++total_rtp_packet_sent_;
}

void RtpRtcpHandler::send_rtp_packet(const char* payload, size_t payload_size, MediaType payload_type) {
    switch (payload_type) {
        case MediaType::H264:
            send_h264(payload, payload_size);
            break;
        case MediaType::AAC:
            send_aac(payload, payload_size);
            break;
        default:
            throw std::runtime_error("Not support media type, broken.");
            break;
    }
    total_payload_size_ += payload_size;
}

bool RtpRtcpHandler::parse_rtcp_packet(const char* buffer, ssize_t received_bytes, RTCPReceiverReport& rr) {
    if (received_bytes < static_cast<int>(sizeof(RTCPHeader))) {
        LOG_WARN("Not enough data for an RTCP packet");
        return false;
    }

    RTCPHeader header;
    memcpy(&header, buffer, sizeof(RTCPHeader));

    if (header.version_ != 2 || header.packet_type_ != 201) {
        LOG_WARN("Invalid RTCP version or not an RTCP RR packet");
        return false;
    }
    rr.version_ = header.version_;
    rr.packet_type_ = header.packet_type_;
    uint16_t length = ntohs(header.length_);
    if (received_bytes < static_cast<int>(sizeof(RTCPHeader) + length * 4)) {
        LOG_WARN("Incomplete RTCP packet");
        return false;
    }
    rr.padding_ = header.padding_;
    rr.length_ = length;
    memcpy(&rr.ssrc_, buffer + sizeof(RTCPHeader), 4);
    rr.ssrc_ = ntohl(rr.ssrc_);

    std::lock_guard<std::mutex> lock(rtcp_send_receive_mutex_);
    for (int i = 0; i < header.report_count_; ++i) {
        RTCPReportBlock rb;
        std::memcpy(&rb, buffer + sizeof(RTCPHeader) + 4 + sizeof(RTCPReportBlock) * i, sizeof(RTCPReportBlock));
//        rb.ssrc_ = ntohl(rb.ssrc_);
//        rb.fraction_lost_ = rb.fraction_lost_;
//        rb.cumulative_packets_lost_ = ntohl(rb.cumulative_packets_lost_) & 0x00FFFFFF;
//        rb.extended_highest_sequence_number_ = ntohl(rb.extended_highest_sequence_number_);
//        rb.interarrival_jitter_ = ntohl(rb.interarrival_jitter_);
//        rb.last_sr_timestamp_ = ntohl(rb.last_sr_timestamp_);
//        rb.delay_since_last_sr_ = ntohl(rb.delay_since_last_sr_);
        rtcp_report_blocks_[rb.ssrc_] = rb;
    }

    return true;
}

void RtpRtcpHandler::receive_rtcp_packet() {
    // Receive and process RTCP packets
    const size_t buffer_size = 4096;
    char buffer[buffer_size];

    struct sockaddr_in client_addr;
    socklen_t client_address_len = sizeof(client_addr);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(rtcp_port_);
    if (inet_pton(AF_INET, ip_.c_str(), &client_addr.sin_addr) <= 0) {
        throw std::runtime_error("RtpRtcpHandler bind client rtcp socket address failed");
    }

    while (is_running_) {
        // Receive an RTCP packet
        ssize_t received_size = recvfrom(rtcp_socket_, buffer, buffer_size, 0,
                                        (struct sockaddr*)&client_addr, &client_address_len);

        // Parse the RTCP packet
        RTCPReceiverReport rr;
        if (received_size > 0 && !parse_rtcp_packet(buffer, received_size, rr)) continue;
        if (rr.packet_type_ == 203) {
            LOG_INFO("Receive BYE, exit rtcp receive thread.");
            break;
        }
    }
}

void RtpRtcpHandler::send_rtcp_packet() {
    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    struct sockaddr_in client_addr;
    socklen_t client_address_len = sizeof(client_addr);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(rtcp_port_);
    if (inet_pton(AF_INET, ip_.c_str(), &client_addr.sin_addr) <= 0) {
        throw std::runtime_error("RtpRtcpHandler bind client rtcp socket address failed");
    }

    RTCPSenderReport sr;
    sr.version_ = 2;
    sr.padding_ = 0;
    sr.packet_type_ = 200;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);
    uint32_t ssrc = dist(mt);
    sr.ssrc_ = htonl(ssrc);
    const uint64_t NTP_epoch = 2208988800ull; // Offset between Unix and NTP epoch (January 1, 1900)

    // Send RTCP packets
    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        uint64_t ntp_time = ((uint64_t)tv.tv_sec + NTP_epoch) << 32;
        ntp_time += (uint64_t)tv.tv_usec * (1ull << 32) / 1000000ull;
        auto ntp_timestamp_msw = (uint32_t)(ntp_time >> 32);
        auto ntp_timestamp_lsw = (uint32_t)(ntp_time & 0xFFFFFFFF);
        sr.ntp_timestamp_msw_ = htonl(ntp_timestamp_msw);
        sr.ntp_timestamp_lsw_ = htonl(ntp_timestamp_lsw);
        sr.rtp_timestamp_ = htonl(timestamp_);
        sr.sender_packet_count_ = htonl(total_rtp_packet_sent_);
        sr.sender_octet_count_ = htonl(total_payload_size_);
        {
            std::lock_guard<std::mutex> lock(rtcp_send_receive_mutex_);
            sr.report_count_ = rtcp_report_blocks_.size();
            sr.length_ = htons(6 + 6 * rtcp_report_blocks_.size());
            memcpy(buffer, &sr, sizeof(RTCPSenderReport));
            int i = 0;
            // Just return back the original Report blocks, meaningless data.
            for (auto [k, v] : rtcp_report_blocks_) {
                memcpy(buffer + sizeof(RTCPSenderReport) + i * sizeof(RTCPReportBlock), &v, sizeof(RTCPReportBlock));
                ++i;
            }
        }
        size_t rtcp_packet_size = sizeof(RTCPSenderReport) + rtcp_report_blocks_.size() * sizeof(RTCPReportBlock);
        ssize_t ret = sendto(rtcp_socket_, buffer, rtcp_packet_size, 0,
                              (struct sockaddr *)&client_addr_, sizeof(client_addr_));
    }
}
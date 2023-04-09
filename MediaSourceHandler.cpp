//
// Created by ricardo on 4/2/23.
//

#include "MediaSourceHandler.h"
#include <iostream>
#include "Logger.h"

H264MediaSource::H264MediaSource(const std::string& url) : url_(url) {}

H264MediaSource::~H264MediaSource() {
    if (h264_file_.is_open()) {
        h264_file_.close();
    }
}

int H264MediaSource::get_next_frame() {
    bool frame_start = false;
    size_t read_bytes;
    while (!h264_file_.eof()) {
        // Read the data from the H.264 file into the buffer
        h264_file_.read(frame_buf_, MAX_FRAME_SIZE);
        read_bytes = h264_file_.gcount();

        for (size_t i = 3; i < read_bytes; ++i) {
            // Check for the start code 0x000001 or 0x00000001
            bool start_code = (i + 2 < read_bytes && frame_buf_[i] == 0x00 && frame_buf_[i + 1] == 0x00 && frame_buf_[i + 2] == 0x01) ||
                              (i + 3 < read_bytes && frame_buf_[i] == 0x00 && frame_buf_[i + 1] == 0x00 && frame_buf_[i + 2] == 0x00 && frame_buf_[i + 3] == 0x01);

            if (start_code) {
                // If we already found the start of a frame, we're now at the start of the next frame
//                LOG_INFO("Start at: {}, {}", i, frame_buf_[i + 4]);
//                if (frame_start) {
                    // Set the file position indicator to the start of the next frame
                    // and resize the vector to remove any trailing data
                    // Every time find a H.264 frame, h264_file_ seek once
                    if (!h264_file_.eof()) h264_file_.seekg(-(read_bytes - i), std::ios_base::cur);
                    // Assume all H.264 file start with a 0x000001 or 0x00000001
                    // So we just need to return where the second start code locate
                    return i;
//                }

//                frame_start = true;
            }
        }
    }
    return -1;
}

std::string H264MediaSource::get_codec_name() const {
    return "H264";
}

MediaType H264MediaSource::get_media_type() const {
    return MediaType::H264;
}

uint8_t H264MediaSource::get_payload_type() const {
    return 96; // H.264 payload type is often set to 96 in dynamic range (96-127)
}

uint32_t H264MediaSource::get_timestamp_increment() const {
    // This value depends on the frame rate of the video.
    // For example, for 30 FPS, the increment would be 90000 / 30 = 3000.
    // Adjust this value according to your specific frame rate.
    // Default is 25FPS.
    return 3600;
}

const char* H264MediaSource::get_frame_buf() const {
    return frame_buf_;
}

std::string H264MediaSource::get_media_sdp() const {
    return "m=video 0 RTP/AVP 96\r\n"
           "a=rtpmap:96 H264/90000\r\n"
           "a=control:stream0\r\n";
}

int H264MediaSource::get_media_fps() const {
    return 25;
}

// media_stream_handler.cpp
void H264MediaSource::open() {
    h264_file_.open(url_, std::ios::binary);
    if (!h264_file_.is_open()) {
        throw std::runtime_error("Failed to open H.264 file: " + url_);
    }
}

void H264MediaSource::close() {
    if (h264_file_.is_open()) {
        h264_file_.close();
    }
}

AACMediaSource::AACMediaSource(const std::string &url) : url_(url) {}

AACMediaSource::~AACMediaSource() {
    if (aac_file_.is_open()) {
        aac_file_.close();
    }
}

int AACMediaSource::get_next_frame() {
    constexpr size_t adts_header_size = 7;
    uint8_t adts_header[adts_header_size];
    aac_file_.read(reinterpret_cast<char *>(adts_header), adts_header_size);
    if (aac_file_.gcount() != adts_header_size) {
        throw std::runtime_error("Read adts header error.");
    } else if (adts_header[0] != 0xFF || (adts_header[1] & 0xF0) != 0xF0) {
        LOG_INFO("adts header: {} {}", adts_header[0], adts_header[1]);
        throw std::runtime_error("adts header format error.");
    }

    size_t frame_size = (((adts_header[3] & 0x03) << 11) | ((adts_header[4] & 0xFF) << 3) | ((adts_header[5] & 0xE0) >> 5)) - 7;
    aac_file_.read(frame_buf_, frame_size);
    if (aac_file_.gcount() != frame_size) return -1;
    return frame_size;
}

std::string AACMediaSource::get_codec_name() const {
    return "AAC";
}

MediaType AACMediaSource::get_media_type() const {
    return MediaType::AAC;
}

uint8_t AACMediaSource::get_payload_type() const {
    return 97;
}

uint32_t AACMediaSource::get_timestamp_increment() const {
    return 1025;
}

const char* AACMediaSource::get_frame_buf() const {
    return frame_buf_;
}

std::string AACMediaSource::get_media_sdp() const {
    return "m=audio 0 RTP/AVP 97\r\n"
           "a=rtpmap:97 mpeg4-generic/44100/2\r\n"
           "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n";
}

int AACMediaSource::get_media_fps() const {
    return 43;
}

// media_stream_handler.cpp
void AACMediaSource::open() {
    aac_file_.open(url_, std::ios::binary);
    if (!aac_file_.is_open()) {
        throw std::runtime_error("Failed to open H.264 file: " + url_);
    }
}

void AACMediaSource::close() {
    if (aac_file_.is_open()) {
        aac_file_.close();
    }
}
//
// Created by ricardo on 4/2/23.
//

#include "MediaStreamHandler.h"
#include <iostream>
#include "Logger.h"

H264MediaSource::H264MediaSource(const std::string& url)
        : url_(url) {

}

H264MediaSource::~H264MediaSource() {
    if (h264_file_.is_open()) {
        h264_file_.close();
    }
}

int H264MediaSource::get_next_frame() {
    bool frame_start = false;
    size_t read_bytes;

    while (true) {
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
                    h264_file_.seekg(-(read_bytes - i), std::ios_base::cur);
                    // Assume all H.264 file start with a 0x000001 or 0x00000001
                    // So we just need to return where the second start code locate
                    return i;
//                }

//                frame_start = true;
            }
        }

        // If we reached the end of the file, break the loop
        if (h264_file_.eof()) {
            break;
        }
    }

    return -1;
}

std::string H264MediaSource::get_codec_name() const {
    return "H264";
}

uint8_t H264MediaSource::get_payload_type() const {
    return 96; // H.264 payload type is often set to 96 in dynamic range (96-127)
}

uint32_t H264MediaSource::get_timestamp_increment() const {
    // This value depends on the frame rate of the video.
    // For example, for 30 FPS, the increment would be 90000 / 30 = 3000.
    // Adjust this value according to your specific frame rate.
    // Assume right now 25FPS
    return 3600;
}

const char* H264MediaSource::get_frame_buf() const {
    return frame_buf_;
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
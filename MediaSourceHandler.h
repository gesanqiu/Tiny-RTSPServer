//
// Created by ricardo on 4/2/23.
//

#ifndef MEDIASTREAMHANDLER_H
#define MEDIASTREAMHANDLER_H


#include <vector>
#include <cstdint>
#include <string>
#include <fstream>

const int MAX_FRAME_SIZE = 512 * 1024;

enum class MediaType : uint8_t {
    H264 = 96,
    AAC = 97
};

class MediaSource {
public:
    virtual ~MediaSource() = default;

    // Get the next media data/frame to be sent to the client
    virtual int get_next_frame() = 0;

    // Get the codec name for the media source (e.g., "H264", "AAC", etc.)
    virtual std::string get_codec_name() const = 0;

    // Get the payload type for the media source (e.g., 96 for H264, 97 for AAC, etc.)
    virtual uint8_t get_payload_type() const = 0;

    // Get the timestamp increment per frame for the media source
    virtual uint32_t get_timestamp_increment() const = 0;

    virtual const char* get_frame_buf() const = 0;

    virtual MediaType get_media_type() const = 0;

    virtual std::string get_media_sdp() const = 0;

    virtual int get_media_fps() const = 0;

    virtual void open() = 0;
    virtual void close() = 0;
};

class H264MediaSource : public MediaSource {
public:
    H264MediaSource(const std::string& url);
    ~H264MediaSource();

    int get_next_frame() override;
    std::string get_codec_name() const override;
    uint8_t get_payload_type() const override;
    uint32_t get_timestamp_increment() const override;
    const char* get_frame_buf() const override;
    MediaType get_media_type() const override;
    std::string get_media_sdp() const override;
    int get_media_fps() const override;
    void open() override;
    void close() override;

private:
    // You can add private member variables and methods for H.264 encoding and frame generation
    std::string url_;
    std::ifstream h264_file_;
    char frame_buf_[MAX_FRAME_SIZE + 10];
};

class AACMediaSource : public MediaSource {
public:
    AACMediaSource(const std::string& url);
    ~AACMediaSource();

    int get_next_frame() override;
    std::string get_codec_name() const override;
    uint8_t get_payload_type() const override;
    uint32_t get_timestamp_increment() const override;
    const char* get_frame_buf() const override;
    MediaType get_media_type() const override;
    std::string get_media_sdp() const override;
    int get_media_fps() const override;
    void open() override;
    void close() override;

private:
    // You can add private member variables and methods for H.264 encoding and frame generation
    std::string url_;
    std::ifstream aac_file_;
    char frame_buf_[MAX_FRAME_SIZE + 10];
};


#endif //MEDIASTREAMHANDLER_H

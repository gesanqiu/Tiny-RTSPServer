//
// Created by root on 4/9/23.
//

#include "MediaSourceManager.h"
#include "Logger.h"

void MediaSourceManager::add_media_source(const std::string& stream_name, const std::string& media_url, const MediaType media_type) {
    switch (media_type) {
        case MediaType::H264:
            media_sources_[stream_name] = std::make_shared<H264MediaSource>(media_url);
            break;
        case MediaType::AAC:
            media_sources_[stream_name] = std::make_shared<AACMediaSource>(media_url);
            break;
        default:
            LOG_ERROR("Media type didn't support");
            break;
    }
}

std::shared_ptr<MediaSource> MediaSourceManager::get_media_source(const std::string& stream_name) {
    auto it = media_sources_.find(stream_name);
    if (it != media_sources_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MediaSourceManager::has_media_source(const std::string& stream_name) {
    return media_sources_.find(stream_name) != media_sources_.end();
}
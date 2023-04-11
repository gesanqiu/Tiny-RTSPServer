//
// Created by root on 4/9/23.
//

#include "MediaSourceManager.h"
#include "Logger.h"

void MediaSourceManager::add_media_source(const std::string& stream_name) {
    media_sources_[stream_name] = std::make_shared<MediaSource>(stream_name);
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
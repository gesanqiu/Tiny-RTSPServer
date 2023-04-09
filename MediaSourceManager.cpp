//
// Created by root on 4/9/23.
//

#include "MediaSourceManager.h"

void MediaSourceManager::add_media_source(const std::string& stream_name, const std::string& media_source) {
    media_sources_[url_prefix_ + stream_name + url_suffix_] = media_source;
}

std::string MediaSourceManager::get_media_source(const std::string& stream_name) {
    auto it = media_sources_.find(stream_name);
    if (it != media_sources_.end()) {
        return it->second;
    }
    return "";
}

bool MediaSourceManager::has_media_source(const std::string& stream_name) {
    auto stream_url = url_prefix_ + stream_name + url_suffix_;
    return media_sources_.find(stream_url) != media_sources_.end();
}

void MediaSourceManager::setup_prefix(const std::string &prefix) {
    url_prefix_ = prefix;
}

void MediaSourceManager::setup_suffix(const std::string &suffix) {
    url_suffix_ = suffix;
}

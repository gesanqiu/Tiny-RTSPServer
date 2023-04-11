//
// Created by root on 4/9/23.
//

#ifndef MEDIASTREAMMANAGER_H
#define MEDIASTREAMMANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include "MediaSourceHandler.h"

class MediaSourceManager {
public:
    static MediaSourceManager& instance() {
        static MediaSourceManager instance;
        return instance;
    }

    void add_media_source(const std::string& stream_name);
    std::shared_ptr<MediaSource> get_media_source(const std::string& stream_name);
    bool has_media_source(const std::string& stream_name);

private:
    MediaSourceManager() = default;
    ~MediaSourceManager() = default;
    MediaSourceManager(const MediaSourceManager&) = delete;
    MediaSourceManager& operator=(const MediaSourceManager&) = delete;

    std::unordered_map<std::string, std::shared_ptr<MediaSource>> media_sources_;
};


#endif //MEDIASTREAMMANAGER_H

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

    void add_media_source(const std::string& stream_name, const std::string& media_source);
    std::string get_media_source(const std::string& stream_name);
    bool has_media_source(const std::string& stream_name);
    void setup_prefix(const std::string& prefix);

    void setup_suffix(const std::string &suffix);

private:
    MediaSourceManager() = default;
    ~MediaSourceManager() = default;
    MediaSourceManager(const MediaSourceManager&) = delete;
    MediaSourceManager& operator=(const MediaSourceManager&) = delete;

    std::string url_prefix_;
    std::string url_suffix_;
    std::unordered_map<std::string, std::string> media_sources_;
};


#endif //MEDIASTREAMMANAGER_H

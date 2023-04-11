#include <iostream>
#include "RtspServer.h"
#include "Logger.h"
#include "MediaSourceManager.h"

int main() {
    LOG_INFO("Hello, World!");
    MediaSourceManager::instance().add_media_source("fire");
    MediaSourceManager::instance().get_media_source("fire")->add_media_track("track0", "/home/ricardo/Downloads/fire.h264", MediaType::H264);
    MediaSourceManager::instance().add_media_source("test_video");
    MediaSourceManager::instance().get_media_source("test_video")->add_media_track("track0", "/home/ricardo/Downloads/test.h264", MediaType::H264);
    MediaSourceManager::instance().add_media_source("test_audio");
    MediaSourceManager::instance().get_media_source("test_audio")->add_media_track("track0", "/home/ricardo/Downloads/test.aac", MediaType::AAC);
    MediaSourceManager::instance().add_media_source("multimedia");
    MediaSourceManager::instance().get_media_source("multimedia")->add_media_track("track0", "/home/ricardo/Downloads/test.h264", MediaType::H264);
    MediaSourceManager::instance().get_media_source("multimedia")->add_media_track("track1", "/home/ricardo/Downloads/test.aac", MediaType::AAC);

    RTSPServer server(8554);
    server.initialize();
    server.run();

    return 0;
}

#include <iostream>
#include "RtspServer.h"
#include "Logger.h"
#include "MediaSourceManager.h"

int main() {
    LOG_INFO("Hello, World!");
    MediaSourceManager::instance().add_media_source("fire", "/home/ricardo/Downloads/fire.h264", MediaType::H264);
    MediaSourceManager::instance().add_media_source("test_video", "/home/ricardo/Downloads/test.h264", MediaType::H264);
    MediaSourceManager::instance().add_media_source("test_audio", "/home/ricardo/Downloads/test.aac", MediaType::AAC);

    RTSPServer server(8554);
    server.initialize();
    server.run();

    return 0;
}

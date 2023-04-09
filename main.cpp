#include <iostream>
#include "RtspServer.h"
#include "Logger.h"
#include "MediaSourceManager.h"

int main() {
    LOG_INFO("Hello, World!");

    MediaSourceManager::instance().setup_prefix("rtsp://127.0.0.1:8554/");
    MediaSourceManager::instance().setup_suffix("/stream0");
    MediaSourceManager::instance().add_media_source("fire", "/home/ricardo/Downloads/fire.h264");
    MediaSourceManager::instance().add_media_source("test", "/home/ricardo/Downloads/test.h264");

    RTSPServer server(8554);
    server.initialize();
    server.run();

    return 0;
}

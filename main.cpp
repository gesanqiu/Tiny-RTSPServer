#include <iostream>
#include "RtspServer.h"
#include "Logger.h"

int main() {
    LOG_INFO("Hello, World!");

    RTSPServer server(8554);
    server.initialize();
    server.run();

    return 0;
}

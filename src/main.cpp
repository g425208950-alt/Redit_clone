#include "server/Server.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [-p|--port <port>]" << std::endl;
}

int main(int argc, char** argv) {
    int port = 6379;
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            port = std::atoi(argv[++i]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Invalid port number" << std::endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    Server server(port);
    server.run();
    return 0;
}
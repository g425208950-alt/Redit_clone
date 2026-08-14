#pragma once
#include <string>
#include <unordered_map>
#include "../resp/Decoder.h"
#include "../resp/Encoder.h"
#include "../store/Store.h"
#include "../commands/Dispatcher.h"

struct Connection {
    int fd = -1;
    bool readClosed = false;
    std::string readBuf;
    std::string writeBuf;
};

class Server {
public:
    explicit Server(int port);
    ~Server();
    void run();
private:
    int port_;
    int listenFd_ = -1;
    int epollFd_ = -1;
    Store store_;
    Dispatcher dispatcher_;
    std::unordered_map<int, Connection> conns_;

    void setupListen();
    void handleAccept();
    void handleRead(int fd);
    void handleWrite(int fd);
    void closeConn(int fd);
    void processRequests(Connection& conn);
    void tryWrite(Connection& conn);
    void modEvents(int fd, uint32_t events);
};
#include "Server.h"

#include <iostream>
#include <cstring>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

Server::Server(int port)
    : port_(port), dispatcher_(store_) {}

Server::~Server() {
    if (listenFd_ != -1) close(listenFd_);
    if (epollFd_ != -1) close(epollFd_);
    for (auto& kv : conns_) close(kv.first);
}

void Server::setupListen() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) { perror("socket"); exit(1); }

    int yes = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(listenFd_, 1024) < 0) { perror("listen"); exit(1); }

    setNonBlocking(listenFd_);
}

void Server::modEvents(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

void Server::run() {
    setupListen();

    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) { perror("epoll_create1"); exit(1); }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev);

    const int MAXEV = 128;
    epoll_event events[MAXEV];
    std::cout << "redis_clone listening on port " << port_ << std::endl;

    while (true) {
        int n = epoll_wait(epollFd_, events, MAXEV, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listenFd_) {
                handleAccept();
                continue;
            }
            if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                closeConn(fd);
                continue;
            }
            if (events[i].events & EPOLLIN) handleRead(fd);
            if (events[i].events & EPOLLOUT) handleWrite(fd);
        }
    }
}

void Server::handleAccept() {
    while (true) {
        sockaddr_in caddr{};
        socklen_t clen = sizeof(caddr);
        int cfd = accept(listenFd_, reinterpret_cast<sockaddr*>(&caddr), &clen);
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        setNonBlocking(cfd);
        int one = 1;
        setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = cfd;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, cfd, &ev);
        conns_[cfd] = Connection{cfd, false, std::string{}, std::string{}};
    }
}

void Server::handleRead(int fd) {
    auto it = conns_.find(fd);
    if (it == conns_.end()) return;

    char buf[65536];
    bool eof = false;
    {
        Connection& conn = it->second;
        while (true) {
            ssize_t r = read(fd, buf, sizeof(buf));
            if (r > 0) {
                conn.readBuf.append(buf, static_cast<size_t>(r));
            } else if (r == 0) {
                eof = true;
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                if (errno == EINTR) continue;
                closeConn(fd);
                return;
            }
        }
        processRequests(conn);
    }

    it = conns_.find(fd);
    if (it == conns_.end()) return;
    Connection& conn = it->second;

    if (eof) conn.readClosed = true;
    if (!conn.writeBuf.empty()) {
        tryWrite(conn);
    } else if (conn.readClosed) {
        closeConn(fd);
    }
}

void Server::handleWrite(int fd) {
    auto it = conns_.find(fd);
    if (it == conns_.end()) return;
    tryWrite(it->second);
}

void Server::processRequests(Connection& conn) {
    Decoder decoder;
    while (!conn.readBuf.empty()) {
        size_t consumed = 0;
        auto opt = decoder.tryParse(conn.readBuf, consumed);
        if (!opt || consumed == 0) break;
        conn.readBuf.erase(0, consumed);

        std::vector<std::string> args;
        if (opt->type == RespType::Array) {
            for (auto& e : opt->items) {
                if (e.type == RespType::BulkString) args.push_back(std::move(e.str));
                else args.push_back("");
            }
        } else {
            conn.writeBuf += Encoder::encode(RespValue::error("ERR protocol error: expected array"));
            continue;
        }

        RespValue reply = dispatcher_.execute(args);
        if (reply.type == RespType::Error && reply.str == "QUIT") {
            conn.writeBuf += Encoder::encode(RespValue::simpleString("OK"));
            closeConn(conn.fd);
            return;
        }
        conn.writeBuf += Encoder::encode(reply);
    }
}

void Server::tryWrite(Connection& conn) {
    while (!conn.writeBuf.empty()) {
        ssize_t w = write(conn.fd, conn.writeBuf.data(), conn.writeBuf.size());
        if (w > 0) {
            conn.writeBuf.erase(0, static_cast<size_t>(w));
        } else if (w < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                modEvents(conn.fd, conn.readClosed ? EPOLLOUT : (EPOLLIN | EPOLLOUT));
                return;
            }
            if (errno == EINTR) continue;
            closeConn(conn.fd);
            return;
        } else {
            break;
        }
    }
    if (conn.writeBuf.empty()) {
        if (conn.readClosed) closeConn(conn.fd);
        else modEvents(conn.fd, EPOLLIN);
    }
}

void Server::closeConn(int fd) {
    auto it = conns_.find(fd);
    if (it != conns_.end()) {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        conns_.erase(it);
    }
}
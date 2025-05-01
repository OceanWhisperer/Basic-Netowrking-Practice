#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

using namespace std;

void make_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool connect_with_timeout(int epfd, int sock, sockaddr* addr, socklen_t addrlen, int timeout = 5) {
    if (connect(sock, addr, addrlen) == 0) {
        return true;  // connected instantly
    }

    if (errno != EINPROGRESS) {
        perror("connect failed immediately");
        return false;
    }

    epoll_event ev{};
    ev.events = EPOLLOUT;
    ev.data.fd = sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        perror("epoll_ctl for connect");
        return false;
    }

    epoll_event events[1];
    int ready = epoll_wait(epfd, events, 1, timeout * 1000);
    if (ready == 0) {
        cerr << "Connection timeout" << endl;
        return false;
    }
    if (ready < 0) {
        perror("epoll_wait error");
        return false;
    }

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        cerr << "Connection failed: " << strerror(err) << endl;
        return false;
    }

    // Optional: remove EPOLLOUT to avoid infinite readiness events
    epoll_ctl(epfd, EPOLL_CTL_DEL, sock, nullptr);
    return true;
}

bool wait_for_response(int epfd, int sock, int timeout = 5) {
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        perror("epoll_ctl wait_response");
        return false;
    }

    epoll_event events[1];
    int ready = epoll_wait(epfd, events, 1, timeout * 1000);
    if (ready == 0) {
        cerr << "Response timeout\n";
        return false;
    }
    if (ready < 0) {
        perror("epoll_wait response");
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <hostname>" << endl;
        return 1;
    }

    const char* host = argv[1];
    const char* port = "80";
    const char* path = "/";

    addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    make_non_blocking(sock);

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return 1;
    }

    if (!connect_with_timeout(epfd, sock, res->ai_addr, res->ai_addrlen, 5)) {
        close(epfd);
        close(sock);
        return 1;
    }

    string req = "GET " + string(path) + " HTTP/1.1\r\n"
                 "Host: " + host + "\r\n"
                 "Connection: close\r\n"
                 "User-Agent: my-cpp-client\r\n\r\n";

    send(sock, req.c_str(), req.size(), 0);

    if (wait_for_response(epfd, sock, 5)) {
        char buffer[4096];
        ssize_t n;
        while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n] = '\0';
            cout << buffer;
        }
    }

    freeaddrinfo(res);
    close(epfd);
    close(sock);
    return 0;
}

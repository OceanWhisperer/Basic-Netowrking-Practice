#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
using namespace std;

void make_non_blocking(int fd)   {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL | O_NONBLOCK);
}

bool wait_for_response(int sock, int timeout = 5) {
    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
        perror("Failed to create epoll instance");
        return false;
    }
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        perror("Event addition error");
        close(epoll_fd);
        return false;
    }
    epoll_event events[1];
    int ready = epoll_wait(epoll_fd, events, 1, timeout*1000);

    if(ready == 0) {
        cerr << "Request Timed Out. No response is 5 seconds" << endl;
        close(epoll_fd);
        return false;
    }
    else if(ready < 0) {
       perror("Epoll wait error");
       return false;
    }
    close(epoll_fd);
    return true;    
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        cerr << "Usage: " << argv[0] << "<hostname>\n" << endl;
        return 1;
    }
    const char * host = argv[1]; // the host name as per command line
    const char * port = "80";
    const char * path = "/";
    
    struct addrinfo hints{}, *res; // define ip and comms in hints, res for traversal
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo returned none");
        return 1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol); // tcp socket for conencting
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) { // make connect
        perror("connect failed");
        return 1;
    }
    make_non_blocking(sock);

    string req = "GET " + std::string(path) + " HTTP/1.1\r\n"
                  "Host: " + host + "\r\n"
                  "User-Agent: my-cpp-client\r\n"
                  "Accept: */*\r\n"
                  "\r\n"; // very basic headers
    send(sock, req.c_str(), req.size(), 0);
    
    if(wait_for_response(sock, 5)) {
        char buffer[4096];
       ssize_t bytes_read;
       while ((bytes_read = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        std::cout << buffer;
       }
    }
    freeaddrinfo(res);
    close(sock);
}
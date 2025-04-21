#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 9000
#define MAX_CLIENTS 1000
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); 
    if (flags == -1) return -1;  
    flags |= O_NONBLOCK;  
    return fcntl(fd, F_SETFL, flags);  
}

using namespace std;

int main() {
    int server_fd, client_fd, epoll_fd;
    sockaddr_in addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }
   
    make_nonblocking(server_fd);
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return -1;
    }

    // Start listening for incoming connections
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        return -1;
    }

    // Create the epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    event.data.fd = server_fd;
    event.events = EPOLLIN;  // Monitor for incoming connections
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl error");
        return -1;
    }

    cout << "Server started running on Port " << PORT << endl;

    while(true) {
        int event_count = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1); // count of events that are cool for io
        for(int i=0; i < event_count; ++i) {
            int cur_fd = events[i].data.fd;

            if(cur_fd == server_fd) {
                while(true) {
                    client_fd = accept(server_fd, NULL, NULL);
                    if(client_fd == -1) {
                        if(errno != EWOULDBLOCK) {
                            perror("error accepting");
                        }
                        break;
                    }
                    make_nonblocking(client_fd);
                    event.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                        perror("epoll_ctl");
                        return -1;
                    }
                    cout << "new Client added Successfully" << endl;

                }

            }
            else if(events[i].events & EPOLLIN) {
                char buffer[BUFFER_SIZE];
                int bytes_read = read(cur_fd, buffer, sizeof(buffer));
                if(bytes_read <= 0) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, NULL);
                    cout << "Client Disconnected" << endl;
                    close(cur_fd);
                }
            }
            else {
                
            }

        }
    }
    return 0;
}
#include <bits/stdc++.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024
#define PORT 12345

unordered_map<int, string> clients_to_usernames;
unordered_map<int, string> clients_to_rooms;
unordered_map<string, unordered_set<int>> rooms_to_clients;
unordered_map<int, queue<string>>pending_messages;

void modify_epoll(int epoll_fd, int client, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = client;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client, &event);
 }

void try_send_pending(int epoll_fd, int client) {
    auto &q = pending_messages[client];
    while(!q.empty()) {
        const string &mesg = q.front();
        ssize_t bytes_sent = send(client, mesg.c_str(), mesg.length(), 0);
        if(bytes_sent == -1) {
            if(errno == EAGAIN or errno == EWOULDBLOCK) return; // meaning still full
            close(client);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client, nullptr);
            return;
        }
        else if(bytes_sent < mesg.size()) {
            q.push(mesg.substr(bytes_sent));
            break;
        }
        q.pop();
    }
    modify_epoll(epoll_fd, client, EPOLLIN | EPOLLET);

}

void set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int create_server_socket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) exit(1);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) exit(1);
    if (listen(server_fd, SOMAXCONN) < 0) exit(1);

    set_non_blocking(server_fd);
    cout << "Server started running on port " << PORT << endl;
    return server_fd;
}

int main() {

    int server_fd = create_server_socket(PORT);
    int epoll_fd = epoll_create1(0);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            int cur_fd = events[i].data.fd;

            if (cur_fd == server_fd) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) continue;

                set_non_blocking(client_fd);

                epoll_event client_event{};
                client_event.events = EPOLLIN | EPOLLET;
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

            } else {
                char buff[BUFFER_SIZE] = {0};
                while(true) {
                 int bytes_read = recv(cur_fd, buff, sizeof(buff) - 1, 0);

                if (bytes_read <= 0) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                    cout << "Client " << clients_to_usernames[cur_fd] << " has disconnected";
                    close(cur_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, nullptr);
                    if (clients_to_rooms.count(cur_fd)) {
                        string room = clients_to_rooms[cur_fd];
                        rooms_to_clients[room].erase(cur_fd);
                    }
                    clients_to_usernames.erase(cur_fd);
                    clients_to_rooms.erase(cur_fd);
                    continue;
                }

                buff[bytes_read] = '\0';
                string message(buff);

                if (!clients_to_usernames.count(cur_fd)) {
                    size_t pos = message.find(':');
                    if (pos == string::npos) {
                        cout << "Bad message from client FD " << cur_fd << ". Client terminated." << endl;
                        close(cur_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, nullptr);
                        continue;
                    }
                    string username = message.substr(0, pos);
                    string room = message.substr(pos + 1);

                    clients_to_usernames[cur_fd] = username;
                    clients_to_rooms[cur_fd] = room;
                    rooms_to_clients[room].insert(cur_fd);

                    cout << username << " joined " << room << endl;

                } else {
                    string username = clients_to_usernames[cur_fd];
                    string room = clients_to_rooms[cur_fd];
                    string full_msg = username + ": " + message;
                    cout << full_msg << endl;
                    for (int client : rooms_to_clients[room]) {
                        if(client == cur_fd)continue;
                        ssize_t bytes_sent = send(client, full_msg.c_str(), full_msg.length(), 0);
                        if(bytes_sent == -1 || bytes_sent < full_msg.length()) {
                           pending_messages[client].push(full_msg.substr(bytes_sent > 0 ? bytes_sent : 0));
                            modify_epoll(epoll_fd, client,EPOLLIN | EPOLLET | EPOLLOUT);
                        }
                    }
                }
                }
            }
            if(events[i].events and EPOLLOUT) {
                try_send_pending(epoll_fd, cur_fd);
            }
        }
    }

    close(server_fd);
    return 0;
}

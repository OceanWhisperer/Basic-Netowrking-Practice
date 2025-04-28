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

struct Client {
    int fd;
    string write_buffer;
    size_t write_offset = 0;
};

unordered_map<int, Client> clients;
unordered_map<int, string> clients_to_usernames;
unordered_map<int, string> clients_to_rooms;
unordered_map<string, unordered_set<int>> rooms_to_clients;

void modify_epoll(int epoll_fd, int client_fd, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
}

void try_send_pending(int epoll_fd, int client_fd) {
    Client &client = clients[client_fd];
    while (client.write_offset < client.write_buffer.size()) {
        ssize_t bytes_sent = send(client.fd, client.write_buffer.data() + client.write_offset,
                                  client.write_buffer.size() - client.write_offset, 0);
        if (bytes_sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return; // Socket still full, wait for next EPOLLOUT
            close(client_fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
            clients.erase(client_fd);
            return;
        }
        client.write_offset += bytes_sent;
    }

    // All data sent successfully
    client.write_buffer.clear();
    client.write_offset = 0;
    modify_epoll(epoll_fd, client_fd, EPOLLIN | EPOLLET);
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

void send_broadcast_message(int epoll_fd, const string &room, int sender_fd, const string &full_msg) {
    for (int client_fd : rooms_to_clients[room]) {
        if (client_fd == sender_fd) continue;
        Client &client = clients[client_fd];

        if (client.write_buffer.empty()) {
            ssize_t bytes_sent = send(client.fd, full_msg.c_str(), full_msg.length(), 0);
            if (bytes_sent == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    clients.erase(client_fd);
                    continue;
                }
                bytes_sent = 0;
            }

            if (bytes_sent < (ssize_t)full_msg.size()) {
                client.write_buffer = full_msg.substr(bytes_sent);
                client.write_offset = 0;
                modify_epoll(epoll_fd, client_fd, EPOLLIN | EPOLLET | EPOLLOUT);
            }
        } else {
            client.write_buffer += full_msg;
        }
    }
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
                clients[client_fd] = {client_fd};

                epoll_event client_event{};
                client_event.events = EPOLLIN | EPOLLET;
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

            } else {
                if (events[i].events & EPOLLIN) {
                    char buff[BUFFER_SIZE] = {0};
                    while (true) {
                        int bytes_read = recv(cur_fd, buff, sizeof(buff) - 1, 0);

                        if (bytes_read <= 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            cout << "Client " << clients_to_usernames[cur_fd] << " has disconnected" << endl;
                            close(cur_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, nullptr);
                            if (clients_to_rooms.count(cur_fd)) {
                                string room = clients_to_rooms[cur_fd];
                                rooms_to_clients[room].erase(cur_fd);
                            }
                            clients_to_usernames.erase(cur_fd);
                            clients_to_rooms.erase(cur_fd);
                            clients.erase(cur_fd);
                            break;
                        }

                        buff[bytes_read] = '\0';
                        string message(buff);

                        if (!clients_to_usernames.count(cur_fd)) {
                            size_t pos = message.find(':');
                            if (pos == string::npos) {
                                cout << "Bad message from client FD " << cur_fd << ". Client terminated." << endl;
                                close(cur_fd);
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cur_fd, nullptr);
                                clients.erase(cur_fd);
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
                            send_broadcast_message(epoll_fd, room, cur_fd, full_msg);
                        }
                    }
                }

                if (events[i].events & EPOLLOUT) {
                    try_send_pending(epoll_fd, cur_fd);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}

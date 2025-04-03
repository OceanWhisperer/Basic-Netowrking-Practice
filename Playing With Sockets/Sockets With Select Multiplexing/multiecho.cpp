#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <fcntl.h>

#define PORT 12345
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
# define MAX_CHATS 100
using namespace std;

void remove_inactive(unordered_map<int,time_t>&last_active, unordered_map<int, string>&clients) {
    time_t now = time(NULL);
    auto it = last_active.begin(); 

    while (it != last_active.end()) {  
        if (now - it->second > 30) {
            int client_fd = it->first;
             
                cout << clients[client_fd] << " 30 seconds idle. Disconnected" << endl;
                close(client_fd);
                clients.erase(it->first);
            

            it = last_active.erase(it);
        }       
        else {
            ++it;
        }
    }
}
void broadcastHistory(vector<string>&chat_history, int client) {
   for(const auto c : chat_history) {
     send(client, c.c_str(), c.length(), 0);
   }
}

void broadcastMessage(const unordered_map<int,string>& clients, const string& message, int current_client) {
    string msg = message;
    for(auto &val : clients) {
        if(val.first == current_client && !val.second.empty()) {
            continue;
        }
        send(val.first, msg.c_str(), msg.length(), 0);
    }
}

void MakeNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        return;
    }
}

int main() {
    int server_fd, new_client_fd;
    struct sockaddr_in server_addy, client_addy;
    socklen_t client_address = sizeof(client_addy);
    fd_set monitor_fds;
    unordered_map<int , string> clients;
    unordered_map<int, time_t>last_active;
    char buffer[BUFFER_SIZE];
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("server creation failed");
        return 1;
    }

    MakeNonBlocking(server_fd);
    server_addy.sin_family = AF_INET;
    server_addy.sin_addr.s_addr = INADDR_ANY;
    server_addy.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(server_fd, (struct sockaddr*)&server_addy, sizeof(server_addy)) < 0) {
       perror("Binding Failed");
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen call Failed");
        return 1;
    }

    cout << "Server started on port " << PORT << endl;

    vector<string>chat_history;

    while(true) {
        FD_ZERO(&monitor_fds);
        FD_SET(server_fd, &monitor_fds);
        int max_fd = server_fd;

        struct timeval tv = {30, 0};

        for(auto &val : clients)  { 
            FD_SET(val.first, &monitor_fds);
            if(val.first > max_fd) max_fd = val.first;
        }
         
        int activity = select(max_fd + 1, &monitor_fds, NULL, NULL, &tv);
        if(activity < 0) {
            perror("Select call Failed");
            return 1;
        }
        if(activity == 0) {
            remove_inactive(last_active, clients);
        }
        
        if(FD_ISSET(server_fd, &monitor_fds)) {
            new_client_fd = accept(server_fd, (struct sockaddr*) &client_addy, &client_address);
            if(new_client_fd < 0) {
                perror("connection failed");
            } else {
                MakeNonBlocking(new_client_fd);
                clients[new_client_fd] = ""; 
                send(new_client_fd, "Enter your username: ", 21, 0);
            }
        }
        
        for(auto it = clients.begin(); it != clients.end();) {
            int client_fd = it->first;
            if(FD_ISSET(client_fd, &monitor_fds)) {
                int bytes_recv = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                if(bytes_recv <= 0) {
                    cout << "Client " + it->second + " has disconnected" << endl;
                    close(client_fd);
                    last_active.erase(client_fd);
                    it = clients.erase(it);
                    continue;
                }

                buffer[bytes_recv] = '\0';

                if(clients[client_fd].empty()) {
                    int len = strlen(buffer);
                    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                        buffer[len - 1] = '\0';
                        len--; // the username is by default stored with \n charecter so we trim it. 
                    }
                    clients[client_fd] = string(buffer);
                    last_active[client_fd] = time(NULL);
                    string welcome_msg = clients[client_fd] + " has joined the chat.\n";
                    cout << welcome_msg << endl;
                    int total_clients = clients.size();
                    total_clients--;
                    chat_history.push_back(welcome_msg);
                    if(chat_history.size() > MAX_CHATS)chat_history.erase(chat_history.begin());
                    if(total_clients < chat_history.size()) {
                        broadcastHistory(chat_history, client_fd);
                    }
                    else {
                        broadcastMessage(clients, welcome_msg, client_fd);
                    }
                } 
                else {
                    int len = strlen(buffer);
                    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                        buffer[len - 1] = '\0';
                        len--; // the normal message is also by default stored with \n.
                    }
                    last_active[client_fd] = time(NULL);         
                    string echo = clients[client_fd] + ": " + buffer + "\n";
                    chat_history.push_back(echo);
                    if(chat_history.size() > MAX_CHATS)chat_history.erase(chat_history.begin());
                    cout << echo << endl;
                    broadcastMessage(clients, echo, client_fd);
                }
            }
            ++it;
        }
    }
    close(server_fd);
    return 0;
}

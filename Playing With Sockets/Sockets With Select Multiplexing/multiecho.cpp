#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <cstring>
#include <map>
#include <fcntl.h>

#define PORT 12345
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024    

using namespace std;

void broadcastMessage(const map<int,string>& clients, const string& message) {
    string msg = message + "\n";  // Ensure newline for proper formatting
    for(auto &val : clients) {  
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
    map<int , string> clients; 
    char buffer[BUFFER_SIZE];
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        cerr << "Failed to create server" << endl;
        return 1;
    }

    MakeNonBlocking(server_fd);
    server_addy.sin_family = AF_INET;
    server_addy.sin_addr.s_addr = INADDR_ANY;
    server_addy.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(server_fd, (struct sockaddr*)&server_addy, sizeof(server_addy)) < 0) {
        cerr << "Could not Bind the server" << endl;
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        cerr << "Error listening on socket" << endl;
        return 1;
    }

    cout << "Server started on port " << PORT << endl;

    while(true) {
        FD_ZERO(&monitor_fds);
        FD_SET(server_fd, &monitor_fds);
        int max_fd = server_fd;

        for(auto &val : clients)  { 
            FD_SET(val.first, &monitor_fds);
            if(val.first > max_fd) max_fd = val.first;
        }
         
        int activity = select(max_fd + 1, &monitor_fds, NULL, NULL, NULL);
        if(activity < 0) {
            cerr << "Select failed" << endl;
            return 1;
        }
        
        if(FD_ISSET(server_fd, &monitor_fds)) {
            new_client_fd = accept(server_fd, (struct sockaddr*) &client_addy, &client_address);
            if(new_client_fd < 0) {
                cerr << "Connection Failed" << endl;
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
                    cout << "Client disconnected" << endl;
                    close(client_fd);
                    it = clients.erase(it);
                    continue;
                }

                buffer[bytes_recv] = '\0';
                string msg(buffer);
                msg.erase(msg.find_last_not_of("\r\n") + 1);

                if(clients[client_fd].empty()) {
                    clients[client_fd] = string(buffer);
                    string welcome_msg = clients[client_fd] + " has joined the chat.";
                    cout << welcome_msg << endl;
                    broadcastMessage(clients, welcome_msg);
                } else {
                    string echo = clients[client_fd] + ": " + buffer;
                    cout << echo << endl;
                    broadcastMessage(clients, echo);
                }
            }
            ++it;
        }
    }
    close(server_fd);
    return 0;
}

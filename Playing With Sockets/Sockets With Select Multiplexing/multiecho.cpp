#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <cstring>
#include <map>
#include <fcntl.h> // helps us modify the behavior of file descriptors

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 10
using namespace std;

void MakeNonBlocking(int server_fd) {
   int flags = fcntl(server_fd, F_GETFL, 0);
   fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, new_client_fd;
    struct sockaddr_in server_addy, client_addy;
    socklen_t client_address = sizeof(client_addy);
    fd_set monitor_fds;
    map<int , string> clients; // maps clients to usernames
    char buffer[BUFFER_SIZE];
    
    // creating main socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        cerr << "Failed to create server" << endl;
        return 1;
    }

    MakeNonBlocking(server_fd);
    server_addy.sin_family = AF_INET;
    server_addy.sin_addr.s_addr = INADDR_ANY;
    server_addy.sin_port = htons(PORT);

    // bind the socket to IP and Port
    if(bind(server_fd, (struct sockaddr*)&server_addy, sizeof(server_addy))<= 0) {
        cerr << "Could not Bind the server" << endl;
        return 1;
    }
    // Start listening on the server
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        cerr << "Error listening on socket" << endl;
        return 1;
    }

    // server started successfully
    cout << "Server started on port " << PORT << endl;

    while(true) {
        FD_ZERO(&monitor_fds); // null our fd set
        FD_SET(server_fd, &monitor_fds); // add main socket to the set
        int max_fd = server_fd;
    }


    return 0;
}
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define PORT 12345

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error\n";
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

   // replace with server IP this is the loopback addy
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported\n";
        return 1;
    }

    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed\n";
        return 1;
    }

    std::cout << "Connected to server. Type a message:\n";

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit") break;

        send(sock, message.c_str(), message.size(), 0);

        ssize_t bytes_received = read(sock, buffer, 1024);
        if (bytes_received <= 0) {
            std::cerr << "Server disconnected\n";
            break;
        }

        std::cout << "Server: " << buffer << "\n";
        memset(buffer, 0, sizeof(buffer));
    }

    close(sock);
    return 0;
}

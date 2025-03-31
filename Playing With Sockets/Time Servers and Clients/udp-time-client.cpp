#include <iostream>
#include <ctime>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

#define PORT 12345
#define MAX_BUFFER_SIZE 1024
using namespace std;

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    socklen_t addr_len = sizeof(server_addr);
    char recv_buffer[MAX_BUFFER_SIZE] = {0};

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        cerr << "Error creating socket" << endl;
        close(client_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address" << endl;
        close(client_fd);
        return 1;
    }

    cout << "Enter message to send (anything will do): ";
    string send_message;
    getline(cin, send_message);

    if (sendto(client_fd, send_message.c_str(), send_message.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error sending the message" << endl;
        close(client_fd);
        return 1;
    }
    cout << "Message sent to server: " << send_message << endl;

    cout << "Waiting for response from server..." << endl;
    ssize_t bytes_received;
    bytes_received = recvfrom(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0, (struct sockaddr*)&server_addr, &addr_len);

    if (bytes_received < 0) {
        cerr << "Error receiving data" << endl;
        close(client_fd);
        return 1;
    } else if (bytes_received > 0) {
        recv_buffer[bytes_received] = '\0';
        cout << "The time received is:\n" << recv_buffer << endl;
    } else {
        cout << "No response received from the server." << endl;
    }

    close(client_fd);
    return 0;
}
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>

#define NTP_SERVER "time.google.com"
#define NTP_PORT 123
#define NTP_PACKET_SIZE 48

using namespace std;

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char packet[NTP_PACKET_SIZE] = {0};

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);

    if (inet_pton(AF_INET, "216.239.35.0", &server_addr.sin_addr) <= 0) {  // Google's NTP IP
        cerr << "Invalid address / Address not supported" << endl;
        return 1;
    }

    packet[0] = 0b11100011; 

    
    if (sendto(sockfd, packet, NTP_PACKET_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Failed to send NTP request" << endl;
        return 1;
    }

    socklen_t addr_len = sizeof(server_addr);
    if (recvfrom(sockfd, packet, NTP_PACKET_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len) < 0) {
        cerr << "Failed to receive response" << endl;
        return 1;
    }

    uint32_t ntp_time;
    memcpy(&ntp_time, &packet[40], sizeof(ntp_time));
    ntp_time = ntohl(ntp_time) - 2208988800U;  

    
    time_t raw_time = (time_t)ntp_time;
    cout << "Server Time: " << ctime(&raw_time);

    close(sockfd);
    return 0;
}
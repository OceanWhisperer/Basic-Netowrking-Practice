#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
using namespace std;

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
    string req = "GET " + std::string(path) + " HTTP/1.1\r\n"
                  "Host: " + host + "\r\n"
                  "User-Agent: my-cpp-client\r\n"
                  "Accept: */*\r\n"
                  "\r\n"; // very basic headers
    send(sock, req.c_str(), req.size(), 0);
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        std::cout << buffer;
    }
    close(sock);
}
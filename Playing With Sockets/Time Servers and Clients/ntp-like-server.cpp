#include <iostream>
#include <ctime>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
    
#define PORT 12345
using namespace std;

int main() {
    int server;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    char buff[1024];
    server = socket(AF_INET, SOCK_DGRAM, 0);
    if(server < 0) {
        cerr << "failed to create socket" << endl;
        return 1;
    }
    memset(&serveraddr, 0 , sizeof serveraddr);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_family = AF_INET;

    if(bind(server, (struct sockaddr*)&serveraddr, sizeof serveraddr)  < 0) {
        cerr << "Failed to bind" << endl;
        return 1;
    }
    cout << "Started Listening on Port " << PORT << "...  " << endl;
    while (true)
    {
        memset(buff, 0, sizeof(buff));
       ssize_t b_read = recvfrom(server, buff, sizeof(buff), 0, (struct sockaddr*)&clientaddr, &addrlen);
       if(b_read < 0) {
        cout << "Error recieving data" << endl;
        continue;
       }
       buff[b_read] = '\0';
       time_t now = time(0);
       string timeStr = ctime(&now);

       sendto(server, timeStr.c_str(), timeStr.length(), 0, (struct sockaddr*)&clientaddr, addrlen);

    }
    close(server);
     

    
    return 0;
}

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define PORT 12345
using namespace std;

int main() {
   int server, new_socket;
   struct sockaddr_in address;
   socklen_t addr_len = sizeof(address);
   char message[1024] = {0};
   server = socket(AF_INET, SOCK_STREAM, 0); 
   if(server == -1) {
    cerr << "Sever creation failed" << endl;
    return 1;
   }
   address.sin_family = AF_INET;
   address.sin_port = htons(PORT);
   address.sin_addr.s_addr = INADDR_ANY;

   if(bind(server, (struct sockaddr*)&address, sizeof address) < 0) {
     cerr << "Binding failed" << endl;
     return 1;
   }
   
   if(listen(server, 3) < 0) {
      cerr << "Listen Failed" << endl;
      return 1;
   }
   std::cout << "Server listening on port " << PORT << "...\n";
   new_socket = accept(server, (struct sockaddr*)&address, &addr_len);
   if(new_socket < 0) {
    cerr << "Accept Failed" << endl;
    return 1;
   }

   while(true) {
    ssize_t bytes_read = read(new_socket, message, 1024);
    if(bytes_read <= 0)break;
    cout << "recieved : " << message << endl;
    send(new_socket, message, bytes_read, 0);
    memset(message, 0 , sizeof(message));

   }
   close(new_socket);
   close(server);

    return 0;
}
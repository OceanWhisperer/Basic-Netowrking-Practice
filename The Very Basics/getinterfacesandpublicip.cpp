// BUILD WITH g++ getinterfacesandpublicip.cpp -o getip -lcurl
// EXECUTE WITH ./ip

extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <string.h>
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <curl/curl.h>
    
}

void get_interface_and_ips() {
    struct ifaddrs *interfaces, *ifa;
    char ip[INET6_ADDRSTRLEN];
    if(getifaddrs(&interfaces) == -1) {
        perror("Error");
        return;
    }
    for(ifa = interfaces; ifa!=NULL; ifa=ifa->ifa_next) {
        int family = ifa->ifa_addr->sa_family;
        if(family == AF_INET) {
            void * addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, ip, sizeof ip);
            printf("Interface: %s | IPv4: %s\n", ifa->ifa_name, ip);
        }
        else {
            void *addr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            inet_ntop(AF_INET6, addr, ip, sizeof ip);
            printf("Interface: %s | IPv6: %s\n", ifa->ifa_name, ip);
        }
    }
    freeifaddrs(interfaces);
    return;

}



void get_public_ip() {
    CURL *curl = curl_easy_init();
    if(!curl) {
        fprintf(stderr, "An Error Occured\n");
        return;
    }
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org?format=text");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return ;
}


int main() {
    printf("Listing all the Network interfaces and their private Ip addresses: ..\n");
    get_interface_and_ips();
    printf("\n");
    printf("The public IP is: ");
    get_public_ip();
    return 0;
}
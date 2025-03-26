extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
}

void get_working_ip(const char *domain) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const char *ports[] = {"443", "80"};

    for (int i = 0; i < 2; ++i) {
        int status = getaddrinfo(domain, ports[i], &hints, &res);
        if (status != 0) continue;

        int cp4 = 1, cp6 = 1;  // Counters for IPv4 & IPv6

        for (p = res; p != NULL; p = p->ai_next) {
            void *addr;
            if (p->ai_family == AF_INET) {
                addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
            } else if (p->ai_family == AF_INET6) {
                addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
            } else {
                continue; // Skip unknown address families
            }

            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            
            int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sockfd == -1) continue;

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
                if (p->ai_family == AF_INET6) {
                    printf("%d. Working IPv6: %s (Port %s)\n", cp6++, ipstr, ports[i]);
                } else if (p->ai_family == AF_INET) {
                    printf("%d. Working IPv4: %s (Port %s)\n", cp4++, ipstr, ports[i]);
                }
            }
            close(sockfd);
        }

        freeaddrinfo(res);  // Free memory after using results
    }

    printf("Finished checking all IPs for %s\n", domain);
}

int main() {
    char domain[256];
    printf("Enter the domain name: ");
    scanf("%255s", domain); // Prevent buffer overflow
    get_working_ip(domain);
    return 0;
}

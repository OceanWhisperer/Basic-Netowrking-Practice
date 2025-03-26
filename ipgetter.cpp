extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <string.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>

}

void resolve_domain(const char *domain, int family) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = family;// ip type 4,6
    hints.ai_socktype = SOCK_STREAM; // tcp
    int status = getaddrinfo(domain, NULL, &hints, &res);
    if(status!=0) return;
    for(p = res; p != NULL; p=p->ai_next) {
        void *addr;
        if(family == AF_INET) {
          addr = &((struct sockaddr_in*)p->ai_addr)->sin_addr;
        }
        else {
            addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
        }
        inet_ntop(family, addr, ipstr, sizeof ipstr);
        printf("Resolved IP is: %s\n", ipstr);    
        freeaddrinfo(res);
        return;
    }
}

int main() {
    char domain[256];
   printf("Enter the domain to resolve ip: ");
    scanf("%s", domain);

    printf("Resolving IP6 .. \n");
    resolve_domain(domain, AF_INET6);
    printf("Resolving IP4 .. \n");
    resolve_domain(domain, AF_INET);
    printf("No address found check for invalid domain");
}
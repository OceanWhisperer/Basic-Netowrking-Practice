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

    printf("The public IP is: ");
    get_public_ip();
    return 0;
}
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;

int main() {
    
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0){
        cerr << "socket error while opening socket" << endl;
        return -1;
    }

    memset((char *) &serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9092);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    const char *message = "Hello world";

    sendto(sockfd, message, strlen(message), 0,
    (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    cout<< "Message sent to server." << endl;


    return 0;
}

#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;
int main(){
    
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0){
        cerr<< "socket error while creating socket" <<endl;
        return -1;
    }

    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9092);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    int status = bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    if(status == 0){
        cout<< "connection established"<<endl;
    }else{
        return -1;
    }

    socklen_t addrLen = sizeof(clientAddr);

    while(1){
        memset((char *) &clientAddr, 0, sizeof(clientAddr));
        char buffer[2048];
        int recive_status = recvfrom(sockfd, buffer, sizeof(buffer), 0, 
            (struct sockaddr *) &clientAddr, &addrLen);

        if(recive_status > 0){
            buffer[recive_status] = '\0';
            cout<< "data recieved : "<<buffer << " : "<< recive_status<<endl;
        }
    }


    return 0;
}
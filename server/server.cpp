#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "../connect.h"
#include <thread>
using namespace std;

int myport = 69;
uint16_t maxbuffersize = 512;

int main(int argc, char **argv){
    
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0){
        cerr<< "socket error while creating socket" <<endl;
        return -1;
    }
    // create the server address
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(myport);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    // bind the server to the address on this machine
    int status = bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    // check if the server is successfully bind the the address or not
    if(status == 0){
        cout<< "connection established"<<endl;
    }else{
        cout << "connection failed to bind"<<endl;
        return -1;
    }
    // calculate the client address length
    socklen_t addrLen = sizeof(clientAddr);
    // initialize the client address variable to store the client address for futher communication
    memset((char *) &clientAddr, 0, sizeof(clientAddr));
    
    char buffer[maxbuffersize]; // Adjust the buffer size accordingly

    while(true){
        // response for packet
        int receive_status = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        uint16_t opcode = getopcode(buffer);
        if(opcode == 1){
            // Create a copy of the client address, buffer, and receive_status using lambda functions
            thread t([clientAddr, &buffer, &receive_status](){
                handleClient(clientAddr, buffer, receive_status);
            }); 
            t.join();
        }
    }

    return 0;
} 
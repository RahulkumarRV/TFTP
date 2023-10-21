#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <utility>
#include "../connect.h"
using namespace std;

int serverport = 69;
uint16_t maxbuffersize = 512;

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        cerr << "socket error while creating socket" << endl;
        return -1;
    }

    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverport); // TFTP default port
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Create an RRQ packet
    uint16_t opcode = htons(1); // Opcode for RRQ
    string filename = "example.txt";
    string mode = "netascii";
    char *buffer;
    pair<char*, size_t> header =  create_RRQ_WRQ_header(opcode, filename, mode);
    int trycount = 3;
    // this while loop will take care if the send request is unsuccessful then it rety and wait for the timeout time for next resend until the limit of retries is reached
    while(trycount-- > 0){
        sendto(sockfd, header.first, header.second,0,  (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        // delete[] header.first;
        if(waitForTimeOut(sockfd, buffer, serverAddr, 1000)){
            // if the server respose to the RRQ then client can start collect the data
            reciveData(sockfd, buffer, serverAddr);
            break;
        }
    }


    return 0;
}

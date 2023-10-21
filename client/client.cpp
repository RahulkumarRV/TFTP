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
    pair<char*, size_t> header =  create_RRQ_WRQ_header(opcode, filename, mode);

    sendto(sockfd, header.first, header.second,0,  (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    delete[] header.first;
    char *buffer;
    if(waitForResponse(sockfd, buffer, serverAddr, 1000, 3)){
        reciveData(sockfd, buffer, serverAddr);
    }


    return 0;
}

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
uint16_t maxbuffersize = 516;

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        cerr << "socket error while creating socket" << endl;
        return -1;
    }
    // create the address of the server accroding the given port and ip address
    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverport); // TFTP default port
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Create an RRQ packet
    uint16_t opcode = htons(RRQ); // Opcode for RRQ
    string filename = "example.txt";
    string mode = "netascii";
    char *buffer;
    pair<char*, size_t> header =  create_RRQ_WRQ_header(opcode, filename, mode);
    int trycount = MAX_RETRY_REQUEST;
    // this while loop will take care if the send request is unsuccessful then it rety and wait for the timeout time for next resend until the limit of retries is reached
    while(trycount-- > 0){
        sendto(sockfd, header.first, header.second,0,  (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        // delete[] header.first;
        struct packet* datapacket = waitForTimeOut(sockfd, buffer, serverAddr, 1000);
        // char* header = creat
        if(datapacket != nullptr) {
            // if the server respose to the RRQ then client can start collect the data
            if(datapacket->opcode == DATA && ntohs(opcode) == RRQ) reciveData(sockfd, datapacket, serverAddr, filename);
            // if server send the ACK and i first send write request it means server ready to connect for writing file
            else if(datapacket->opcode == ACK && ntohs(opcode) == WRQ){
                handleClient(serverAddr, "example.txt", MAX_PACKET_SIZE);
            }
            break;
        }
    }

    // if trycount is 0 then it mean connection not working or disconnected
    if(trycount <= 0){
        cout << "Connection not working" << endl;
        return 0;
    }
    





    return 0;
}

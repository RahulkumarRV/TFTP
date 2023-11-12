#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <cstring>
#include <utility>
#include "../connect.h"
using namespace std;

int serverport = 69;
uint16_t maxbuffersize = 516;

int main(int argc, char *argv[]) {
    // if the user enter the in valid command line arguments then throw error
    if(argc < 3){
        cout << "Invalid arguments" << endl;
        return 1;
    }
    char* dp = argv[2];
    
    // by default server ip address
    const char* IP_address = "127.0.0.1";
    // check if user passed the ip address in command line arguments then set the server ip address to passed value
    for(int i=0; i<argc; i++){
        if(strcmp(argv[i], "-ip")==0 && i + 1 < argc){
            IP_address = argv[i+1];
            break;
        } 
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) { cerr << "socket error while creating socket" << endl; return -1; }
    // create the address of the server accroding the given port and ip address
    struct sockaddr_in serverAddr;
    initializeAddressWithIPAndPort(serverAddr, serverport, IP_address);
    // Create an RRQ packet
    uint16_t opcode = 0; // Opcode for RRQ
    if(strcmp(argv[1], "READ")==0) opcode = htons(RRQ);
    else if(strcmp(argv[1], "WRITE")==0) opcode = htons(WRQ);
    else if(strcmp(argv[1], "DIR") == 0) opcode = htons(DIR);
    else if(strcmp(argv[1], "MDIR") == 0) opcode = htons(MDIR);
    else {cout << "Invalid operation"<<endl; return 1;}
    // file name from the command line arguments 
    const char* filename = argv[2];

    // if(ntohs(opcode) == WRQ && isCompressedFileExist(filename)) {
    //     cout << "filnaem : " << filename << endl;
    //     filename = changeExtension(filename, ".bin").c_str();
    // }
    // the mode of transmission
    string mode = "netascii"; 
    char *buffer;

    pair<char*, size_t> header =  create_RRQ_WRQ_header(opcode, filename, mode);
    int trycount = MAX_RETRY_REQUEST;
    // this while loop will take care if the send request is unsuccessful then it rety and wait for the timeout time for next resend until the limit of retries is reached
    while(trycount-- > 0){
        sendto(sockfd, header.first, header.second,0,  (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        // delete[] header.first;
        struct packet* datapacket = waitForTimeOut(sockfd, serverAddr, 1000);
        // char* header = creat
        if(datapacket != nullptr) {
            // if the server respose to the RRQ as data packet then client can start collect the data
            if(datapacket->opcode == DATA && ntohs(opcode) == RRQ) {
                reciveData(sockfd, datapacket, serverAddr, filename);
                // isDecompressedFileExist(filename);
            }
            else if(datapacket->opcode == DATA && ntohs(opcode) == DIR) reciveData(sockfd, datapacket, serverAddr, "", false, true);
            // if server send the ACK and i first send write request it means server ready to connect for collecting file data
            else if(datapacket->opcode == ACK && ntohs(opcode) == WRQ){
                handleServer(serverAddr, filename, sockfd);
            }
            else if(datapacket->opcode == ACK && ntohs(opcode) == MDIR) cout << "Directory created successfully" << endl;
            // server send the error packet then shout down this connection
            else if(datapacket->opcode == ERROR){
                cout << "[ERROR] " << datapacket->data << endl;
                return -1;
            }
            else {
                sendError( 4, errorCodes[4], sockfd, serverAddr);
                cout << "[ERROR] " << errorCodes[4] << endl;
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

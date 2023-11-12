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
uint16_t maxbuffersize = 516;

int main(int argc, char **argv){
    const char* IP_address = "127.0.0.1";
     // check if user passed the ip address in command line arguments then set the server ip address to passed value
    for(int i=0; i<argc; i++){
        if(strcmp(argv[i], "-ip")==0 && i + 1 < argc){
            IP_address = argv[i+1];
            break;
        } 
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){ cerr<< "socket error while creating socket" <<endl; return -1; }
    // create the server address
    struct sockaddr_in serverAddr, clientAddr;
    initializeAddressWithIPAndPort(serverAddr, myport, IP_address);
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
    char *filename;
    while(true){
        // response for packet
        int receive_status = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        uint16_t opcode = getopcode(buffer);
        cout << buffer + sizeof(uint16_t) << endl;
        filename = buffer + sizeof(uint16_t);
        if(opcode == DIR){
            if(fs::exists(filename)) generateDirectory(filename);
            else {
                sendError(2, errorCodes[2], sockfd, clientAddr);
                return 1;
            }
        } 
        if(opcode == RRQ || opcode == DIR){
            // Create a copy of the client address, buffer, and receive_status using lambda functions
            thread t([clientAddr, &buffer, &receive_status](){
                handleClient(clientAddr, buffer + sizeof(uint16_t), getopcode(buffer), receive_status);
            }); 
            t.detach();
        }else if(opcode == WRQ){
            // handle the client to write data on the server
            thread t([clientAddr, &buffer, &receive_status](){
                handleClientToWriteFileOnServer(clientAddr, buffer, receive_status);
            });
            t.detach();
        }else if(opcode == MDIR){
            try{
                fs::create_directories(filename);
                pair<char*, size_t> packet = create_ACK_header(ACK, 0);
                sendto(sockfd, packet.first, packet.second, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
            }catch(const std::exception &e){
                sendError(6, errorCodes[6], sockfd, clientAddr);
            }
        }
    }

    return 0;
} 
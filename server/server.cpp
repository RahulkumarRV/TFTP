#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "../connect.h"
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

    memset((char *) &clientAddr, 0, sizeof(clientAddr));
    

    char buffer[512]; // Adjust the buffer size accordingly

    while(true){
        // response for packet
        int receive_status = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
        
        if(receive_status < 0){
            cerr<< "recvfrom failed"<<endl;
            break;
        }
        if(receive_status == 0){
            cout << "connection closed"<<endl;
            break;
        }
        // Extract the opcode from the received data (first 2 bytes)
        uint16_t opcode;
        uint16_t blocknumber;
        char *data;
        uint16_t data_size = parse_DATA_header(buffer, opcode, blocknumber, data, receive_status);
        cout << "Opcode : " << opcode << endl;
        cout << "block number : " << blocknumber << endl;
        data[data_size] = '\0';
        cout << "data : " << data << endl;
        cout << "data size : " << strlen(data) << endl;
        cout << "recived number of bytes : " << receive_status<<endl;
        
        if(receive_status < 512){
            break;
        }
         
    }






    return 0;
} 
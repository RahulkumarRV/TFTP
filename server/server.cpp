#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;

// Structure to represent the RRQ packet header
struct tftphdr {
    short th_opcode;   // Opcode (Read Request: 1)
    string th_stuff;   // Filename, mode, and null bytes
};

void parse_RRQ_WRQ_header(char* header, uint16_t& opcode, char*& filename, char*& mode) {
    // Extract opcode
    memcpy(&opcode, header, 2);
    opcode = ntohs(opcode);

    // Set filename and mode pointers
    filename = header + 2;
    mode = filename + strlen(filename) + 1;
}

void parse_ACK_header(char* header, uint16_t &opcode, uint16_t &blocknumber){
    memcpy(&opcode, header, 2);
    opcode = ntohs(opcode);
    memcpy(&blocknumber, header + 2, 2);
    blocknumber = ntohs(blocknumber);
}

void parse_ERROR_header(char* header, uint16_t &opcode, uint16_t &errorcode, char*& errormessage){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&errorcode, header + sizeof(opcode), sizeof(errorcode));
    errorcode = ntohs(errorcode);
    errormessage = header + sizeof(opcode) + sizeof(errorcode);
}

uint16_t parse_DATA_header(char* header, uint16_t& opcode, uint16_t& blocknumber, char*& data, size_t packetSize) {
    // Extract opcode
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);

    // Extract block number
    memcpy(&blocknumber, header + sizeof(opcode), sizeof(blocknumber));
    blocknumber = ntohs(blocknumber);

    // Calculate the length of the data portion based on the packet size and header length
    size_t headerSize = sizeof(opcode) + sizeof(blocknumber);
    size_t dataLength = packetSize - headerSize;

    // Set the data pointer
    data = header + headerSize;

    return dataLength;
}



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
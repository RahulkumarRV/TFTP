#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
using namespace std;

#define MAX_PACKET_SIZE 512

enum packet_type {
    RRQ = 1,
    WWQ,
    DATA,
    ACK,
    ERROR
}PACKET_TYPE;

// create the rrq or wrq header which will contains the opcode, file name and mode of transfer
pair<char*, size_t> create_RRQ_WRQ_header(uint16_t opcode, const string& filename, const string& mode) {
    // Calculate the buffer size
    size_t buffer_size = sizeof(opcode) + filename.length() + 1 + mode.length() + 1;
    // Allocate memory for the buffer
    char* buffer = new char[buffer_size];
    // Copy data into the buffer
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), filename.c_str(), filename.length());
    buffer[sizeof(opcode) + filename.length()] = 0; // Null separator
    memcpy(buffer + sizeof(opcode) + filename.length() + 1, mode.c_str(), mode.length());
    buffer[buffer_size - 1] = 0; // Null terminator
    // return the buffer containing header and it's size 
    return make_pair(buffer, buffer_size);
}

// create the acknowledgement header which will contains the opcode and block number 
pair<char*, size_t> create_ACK_header(uint16_t opcode, uint16_t blocknumber) {
    uint16_t size = sizeof(opcode) + sizeof(blocknumber);
    char* buffer = new char[size];
    size_t buffer_size = size;
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blocknumber, sizeof(blocknumber));
    return make_pair(buffer, buffer_size);
}

// create the data header of the tftp header which contains the opcode and error information
pair<char*, size_t> create_ERROR_header(uint16_t opcode, uint16_t errorcode, string errormessage){
    uint16_t size = sizeof(opcode) + sizeof(errorcode);
    size_t buffer_size = size + errormessage.length() + 1;
    char *buffer = new char[buffer_size];
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &errorcode, sizeof(errorcode));
    memcpy(buffer + size, errormessage.c_str(), errormessage.length());
    buffer[buffer_size] = '\0';

    return make_pair(buffer, buffer_size);
}

// create the data header of the tftp, which will be contains the opcode, block number and data
pair<char*, size_t> create_DATA_header(uint16_t opcode, uint16_t blocknumber, string data){
    // 2 bytes for opcode, 2 bytes for block number, therfore that actual data should not be greater that 518 bytes in each packet
    uint16_t datasize = MAX_PACKET_SIZE - sizeof(opcode) - sizeof(blocknumber);
    if(data.length() > datasize){
        cout << "data length exceeds 518 bytes";
        return make_pair(nullptr, -1);
    } 
    uint16_t size = sizeof(opcode) + sizeof(blocknumber);
    size_t buffer_size = size + data.length();

    char *buffer = new char[buffer_size];
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blocknumber, sizeof(blocknumber));
    memcpy(buffer + size, data.c_str(), data.length());
    // data is null character terminated if and only if it's size is lesser thatn 518 bytes
    if(data.length() < datasize){
        buffer[buffer_size] = '\0';
    }
    return make_pair(buffer, buffer_size);
}

// parse the rrq and wrq header of the packet
void parse_RRQ_WRQ_header(char* header, uint16_t& opcode, char*& filename, char*& mode) {
    // Extract opcode
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);

    // Set filename and mode pointers
    filename = header + sizeof(opcode);
    mode = filename + strlen(filename) + 1;
}

// parse the acknowledgment header of tftp to get the opcode and block number
void parse_ACK_header(char* header, uint16_t &opcode, uint16_t &blocknumber){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&blocknumber, header + sizeof(opcode), sizeof(blocknumber));
    blocknumber = ntohs(blocknumber);
}

// parse the tftp header to extract the error code information 
void parse_ERROR_header(char* header, uint16_t &opcode, uint16_t &errorcode, char*& errormessage){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&errorcode, header + sizeof(opcode), sizeof(errorcode));
    errorcode = ntohs(errorcode);
    errormessage = header + sizeof(opcode) + sizeof(errorcode);
}

// parse the tftp header to extract the opcode, block number and data 
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

// the the opcode of the tftp header from the response buffer
uint16_t getopcode(char buffer[]){
    uint16_t opcode;
    memcpy(&opcode, buffer, sizeof(opcode));
    return ntohs(opcode);
}

// wait for the response until timeout is reached
// if response is come before the timeout then return true and pass the result in parms which pass as refrence
// else return false
bool waitForTimeOut(int sockfd, char*& buffer, struct sockaddr_in& address, int timeout) {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;        // seconds
    tv.tv_usec = (timeout % 1000) * 1000;  // microseconds
    int bufferSize = sizeof(buffer);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    int selectResult = select(sockfd + 1, &fds, NULL, NULL, &tv);

    if (selectResult < 0) {
        std::cerr << "select failed" << std::endl;
        return false;
    } else if (selectResult == 0) {
        // Timeout
        return false;
    } else {
        // Data available to read
        char databuffer[MAX_PACKET_SIZE];
        socklen_t addressLen = sizeof(address);
        int bytesReceived = recvfrom(sockfd, databuffer, sizeof(databuffer), 0, (struct sockaddr*)&address, &addressLen);
        uint16_t opcode, blocknumber;
        char *data;
        buffer = databuffer;
        parse_DATA_header(databuffer, opcode, blocknumber, data, bytesReceived);
        cout << " checkt response : " << data << " opcode " << opcode << " block number " << blocknumber << " recived bytes " << bytesReceived << endl;
        if (bytesReceived < 0) {
            std::cerr << "recvfrom failed" << std::endl;
            return false;
        }
        return true; 
    }
}


void reciveData(int sockfd, char*& buffer, struct sockaddr_in& address){
    uint16_t offset = 0, number_of_bytes = 0;
    bool moreDataAvailable = true;
    sockaddr_in newAddress;
    memset((char *)&address, 0, sizeof(address));
    uint16_t opcode, errorcode, blocknumber;
    while(moreDataAvailable){
        opcode = getopcode(buffer);
        if(opcode == DATA){
            // if the comming packet is data packet then store the data and send the ACK for this packet


        }else{
            
            
            break;
        }
        moreDataAvailable = false;
    }
}


int generateRandomPortAndBind(int minPort, int maxPort, struct sockaddr_in& addr, int sockfd) 
{
    int countoftry = 100;
    int portNumber = minPort;
    while(countoftry > 0){
        portNumber = minPort + (rand() % (maxPort - minPort + 1));
        addr.sin_port = htons(portNumber);
        if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == 0){
            return portNumber;
        }
    }
    return -1;
}


void handleClient(struct sockaddr_in clientAddr, char* buffer, int receiveStatus) {
    char clientIP[INET_ADDRSTRLEN]; // client IP address
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    uint16_t clientPort = ntohs(clientAddr.sin_port); // client port
    struct sockaddr_in myAddress;
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset((char *)&myAddress, 0, sizeof(myAddress));
    myAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &myAddress.sin_addr);
    int myPort = generateRandomPortAndBind(5000, 6000, myAddress, socketfd);
    if( myPort == -1) {
        cout << "Thread not able to bind to a new port" << endl;
        return;
    }else{
        cout << "Thread successfully bind to a new port " <<  myPort << " for the client " << clientIP << endl;
    }
    string message = "server new port address";
    socklen_t addrLen = sizeof(clientAddr);
    uint16_t opcode = htons(3), blocknumber = htons(1);
    pair<char*, size_t> packet = create_DATA_header(opcode, blocknumber, "rahul kumar");
    sendto(socketfd, packet.first, packet.second, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    
}
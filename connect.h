#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;

#define MAX_PACKET_SIZE 512

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

pair<char*, size_t> create_ACK_header(uint16_t opcode, uint16_t blocknumber) {
    uint16_t size = sizeof(opcode) + sizeof(blocknumber);
    char* buffer = new char[size];
    size_t buffer_size = size;
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blocknumber, sizeof(blocknumber));
    return make_pair(buffer, buffer_size);
}

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


void parse_RRQ_WRQ_header(char* header, uint16_t& opcode, char*& filename, char*& mode) {
    // Extract opcode
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);

    // Set filename and mode pointers
    filename = header + sizeof(opcode);
    mode = filename + strlen(filename) + 1;
}

void parse_ACK_header(char* header, uint16_t &opcode, uint16_t &blocknumber){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&blocknumber, header + sizeof(opcode), sizeof(blocknumber));
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

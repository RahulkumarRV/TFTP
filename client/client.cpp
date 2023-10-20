#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <utility>

using namespace std;

pair<char*, size_t> create_RRQ_WRQ_header(uint16_t opcode, const string& filename, const string& mode) {
    // Calculate the buffer size
    size_t buffer_size = 2 + filename.length() + 1 + mode.length() + 1;
    
    // Allocate memory for the buffer
    char* buffer = new char[buffer_size];
    
    // Copy data into the buffer
    memcpy(buffer, &opcode, 2);
    memcpy(buffer + 2, filename.c_str(), filename.length());
    buffer[2 + filename.length()] = 0; // Null separator
    memcpy(buffer + 2 + filename.length() + 1, mode.c_str(), mode.length());
    buffer[buffer_size - 1] = 0; // Null terminator
    // return the buffer containing header and it's size 
    return make_pair(buffer, buffer_size);
}

pair<char*, size_t> create_ACK_header(uint16_t opcode, uint16_t blocknumber) {
    char* buffer = new char[4];
    size_t buffer_size = 4;
    memcpy(buffer, &opcode, 2);
    memcpy(buffer + 2, &blocknumber, 2);

    return make_pair(buffer, buffer_size);
}

pair<char*, size_t> create_ERROR_header(uint16_t opcode, uint16_t errorcode, string errormessage){
    size_t buffer_size = 4 + errormessage.length() + 1;

    char *buffer = new char[buffer_size];
    memcpy(buffer, &opcode, 2);
    memcpy(buffer + 2, &errorcode, 2);
    memcpy(buffer + 4, errormessage.c_str(), errormessage.length());
    buffer[buffer_size] = '\0';

    return make_pair(buffer, buffer_size);
}

pair<char*, size_t> create_DATA_header(uint16_t opcode, uint16_t blocknumber, string data){
    // 2 bytes for opcode, 2 bytes for block number, therfore that actual data should not be greater that 518 bytes in each packet
    if(data.length() > 508){
        cout << "data length exceeds 518 bytes";
        return make_pair(nullptr, -1);
    } 
    size_t buffer_size = 4 + data.length();

    char *buffer = new char[buffer_size];
    memcpy(buffer, &opcode, 2);
    memcpy(buffer + 2, &blocknumber, 2);
    memcpy(buffer + 4, data.c_str(), data.length());
    // data is null character terminated if and only if it's size is lesser thatn 518 bytes
    if(data.length() < 508){
        buffer[buffer_size] = '\0';
    }
    return make_pair(buffer, buffer_size);
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        cerr << "socket error while creating socket" << endl;
        return -1;
    }

    memset((char*)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9092); // TFTP default port
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Create an RRQ packet
    // uint16_t opcode = htons(2); // Opcode for RRQ
    // string filename = "example.txt";
    // string mode = "netascii";
    // pair<char*, size_t> header =  create_RRQ_WRQ_header(opcode, filename, mode);

    // Create a ACK packet
    // uint16_t opcode = htons(4); // Opcode for ACK
    // uint16_t blocknumber = htons(3000); // Block number for ACK
    // pair<char*, size_t> header = create_ACK_header(opcode, blocknumber);

    // Create a error packet
    // uint16_t opcode = htons(5);
    // uint16_t errorcode = htons(3);
    // string errormessage = "Unknown file";
    // pair<char*, uint16_t> header = create_ERROR_header(opcode, errorcode, errormessage);


    // Create a data packet
    uint16_t opcode = htons(3);
    uint16_t blocknumber = htons(645);
    string data = "hey, my name is rahul, i create this client server for tftp okeokd ,ince TFTP includes no login or access control mechanisms, care must be taken in the rights granted to a TFTP server process so as not to violate the security of the server hosts file system.  TFTP is often installed with controls such that only files that have public read access are available via TFTP and writing files via TFTP is disallowed installed with controls such that only files that have public read access are available via TFTP and writing files via TFTP is disallowed";
    uint16_t offset = 0, number_of_bytes = 0;

    while(number_of_bytes < data.length()){
        offset = number_of_bytes;
        number_of_bytes += data.length() < 508 ? data.length() : 508;
        cout << data.substr(offset, number_of_bytes) << endl;
        pair<char*, uint16_t> header = create_DATA_header(opcode, blocknumber, data.substr(offset, number_of_bytes));
        cout<<" header size : " << header.second << endl;
        sendto(sockfd, header.first, header.second, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        delete[] header.first; // Clean up the buffer
    }

    

    return 0;
}

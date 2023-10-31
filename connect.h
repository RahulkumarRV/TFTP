#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <filesystem>
#include <unordered_map>
using namespace std;

#define MAX_PACKET_SIZE 516 // sotre the maximum packet size including the header and data 
#define MAX_RETRY_REQUEST 3
#define MAX_ITERATIONS 32768
// use as palceholder for the type of the request packet
enum packet_type {
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERROR
}PACKET_TYPE;

// store the error code with it's message
unordered_map<int, string> errorCodes = {
        {0, "Not defined, see error message (if any)."},
        {1, "File not found."},
        {2, "Access violation."},
        {3, "Disk full or allocation exceeded."},
        {4, "Illegal TFTP operation."},
        {5, "Unknown transfer ID."},
        {6, "File already exists."},
        {7, "No such user."}
    };

struct packet {
    uint16_t opcode;
    uint16_t error_code;
    uint16_t blocknumber;
    char *data;
    uint16_t packet_length;
};

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
    opcode = htons(opcode);
    blocknumber = htons(blocknumber);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blocknumber, sizeof(blocknumber));
    return make_pair(buffer, buffer_size);
}

// create the data header of the tftp header which contains the opcode and error information
pair<char*, size_t> create_ERROR_header(uint16_t opcode, uint16_t errorcode, string errormessage){
    uint16_t size = sizeof(opcode) + sizeof(errorcode);
    size_t buffer_size = size + errormessage.length() + 1;
    char *buffer = new char[buffer_size];
    opcode = htons(opcode);
    errorcode = htons(errorcode);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &errorcode, sizeof(errorcode));
    memcpy(buffer + size, errormessage.c_str(), errormessage.length());
    buffer[buffer_size] = '\0';

    return make_pair(buffer, buffer_size);
}

// create the data header of the tftp, which will be contains the opcode, block number and data
pair<char*, size_t> create_DATA_header(uint16_t opcode, uint16_t blocknumber, char* data){
    // 2 bytes for opcode, 2 bytes for block number, therfore that actual data should not be greater that 518 bytes in each packet
    uint16_t datasize = MAX_PACKET_SIZE - sizeof(opcode) - sizeof(blocknumber);
    int dataLength = strlen(data);
    if(dataLength > datasize){
        cout << "data length exceeds 518 bytes";
        return make_pair(nullptr, -1);
    } 
    opcode = htons(opcode);
    blocknumber = htons(blocknumber);
    uint16_t size = sizeof(opcode) + sizeof(blocknumber);

    size_t buffer_size = size + dataLength;
    char *buffer = (char *) malloc(buffer_size);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blocknumber, sizeof(blocknumber));
    memcpy(buffer + size, data, dataLength);
    // data is null character terminated if and only if it's size is lesser thatn 518 bytes
    if(dataLength < datasize){
        buffer[buffer_size] = '\0';
    }
    return make_pair(buffer, buffer_size);
}

// parse the rrq and wrq header of the packet
void parse_RRQ_WRQ_header(char* header, uint16_t& opcode, char*& filename, char*& mode) {
    // Extract opcode
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);

    // Set filename pointer
    char* char_ptr = header + sizeof(opcode);
    filename = (char*) malloc(strlen(char_ptr) + 1);
    strcpy(filename, char_ptr);

    // Set mode pointer
    char_ptr = filename + strlen(char_ptr) + 1;
    mode = (char*) malloc(strlen(char_ptr) + 1);
    strcpy(mode, char_ptr);
}


// parse the acknowledgment header of tftp to get the opcode and block number
void parse_ACK_header(char* header, uint16_t &opcode, uint16_t &blocknumber){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&blocknumber, header + sizeof(opcode), sizeof(blocknumber));
    blocknumber = ntohs(blocknumber);
}

// parse the tftp header to extract the error code information 
void parse_ERROR_header(char* header, uint16_t &opcode, uint16_t &errorcode, char*& errormessage, size_t packetSize){
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&errorcode, header + sizeof(opcode), sizeof(errorcode));
    errorcode = ntohs(errorcode);
    int headerSize = sizeof(opcode) + sizeof(errorcode);
    int dataLength  = packetSize - headerSize;
    errormessage = (char *)malloc(dataLength);
    memcpy(errormessage, header + headerSize, dataLength);
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
    data = (char *)malloc(dataLength);
    memcpy(data, header + headerSize, dataLength);

    return dataLength;
}

// the the opcode of the tftp header from the response buffer
uint16_t getopcode(char buffer[]){
    uint16_t opcode;
    memcpy(&opcode, buffer, sizeof(opcode));
    return ntohs(opcode);
}



void sendError(int code, string message, int socketfd, const sockaddr_in addr){
    pair<char*, size_t> errorpacket = create_ERROR_header(ERROR, code, message);
    sendto(socketfd, errorpacket.first, errorpacket.second, 0, (struct sockaddr *)&addr, sizeof(addr));
}

// check the address's are equal by IP and socket address
bool areSockAddressesEqual(const sockaddr_in& addr1, const sockaddr_in& addr2) {
    // Compare the IP address
    if (addr1.sin_addr.s_addr != addr2.sin_addr.s_addr) {
        return false;
    }
    // Compare the port
    if (addr1.sin_port != addr2.sin_port) {
        return false;
    }
    // The addresses are the same
    return true;
}

// wait for the response until timeout is reached
// if response is come before the timeout then return true and pass the result in parms which pass as refrence
// else return false
struct packet* waitForTimeOut(int sockfd, char*& buffer, struct sockaddr_in& address, int timeout) {
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
        return nullptr;
    } else if (selectResult == 0) {
        // Timeout
        return nullptr;
    } else {
        // Data available to read
        char databuffer[MAX_PACKET_SIZE];
        socklen_t addressLen = sizeof(address);
        int bytesReceived = recvfrom(sockfd, databuffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&address, &addressLen);
        struct packet *newPacket = (struct packet*) malloc(sizeof(struct packet));
        // block number will be as error code if the opcode is ERROR and error message is data
        parse_DATA_header(databuffer, newPacket->opcode, newPacket->blocknumber, newPacket->data, bytesReceived);
        newPacket->packet_length = bytesReceived;
        // cout<< newPacket->data << " data size : " << strlen(newPacket->data) << endl;
        if (bytesReceived < 0) {
            std::cerr << "recvfrom failed" << std::endl;
            return nullptr;
        }
        return newPacket; 
    }
}

// it handle the data coming for the requested rrq request
void reciveData(int sockfd, struct packet*& buffer, struct sockaddr_in& address, string filename){
    uint16_t offset = 0, number_of_bytes = 0;
    bool moreDataAvailable = true;
    sockaddr_in newAddress;
    memset((char*)&newAddress, 0, sizeof(sockaddr_in));
    socklen_t addrLen = sizeof(newAddress);
    ofstream outputfile;
    // memset((char *)&address, 0, sizeof(address));
    uint16_t opcode, errorcode, blocknumber;
    char *data;
    int trycount = 3;
    if (filesystem::exists(filename)) {
        cout << "File " << filename << " already exists" << endl;
        sendError(6, errorCodes[6], sockfd, address);
        return;
    }
    // open the output file with given name
    outputfile.open(filename);
    while(moreDataAvailable){
        opcode = buffer->opcode;
        cout << opcode << " " << buffer->blocknumber << endl;
        if(opcode == DATA ){
            // if the comming packet is data packet then store the data and send the ACK for this packet
            if(outputfile.is_open()){
                outputfile << buffer->data;

            }else{
                // fail to open the file due to file not exist
                if(outputfile.rdstate() && ios_base::failbit){
                    sendError( 1, errorCodes[1], sockfd, address);
                    cout << "File not found"<<endl;
                }
                return;
            }
        }else{
            // expecting DATA packet but recived acknowledgement so ignore it may be server sended it pay mistake
            if(buffer->opcode == ACK){
                continue;
            }
            // recived the error packet so print and terminate connection
            else{
                cout <<"[ERROR] " << buffer->data << endl;
                break;
            }
        }
        cout << "buffer packet size : " << buffer->packet_length << endl;
        if(buffer->packet_length < MAX_PACKET_SIZE){
            moreDataAvailable = false;
            outputfile.close();
            pair<char*, size_t> ack_packet = create_ACK_header(ACK, buffer->blocknumber);
            sendto(sockfd, ack_packet.first, ack_packet.second, 0, (struct sockaddr *)&address, sizeof(address));
        }else{
            trycount = MAX_RETRY_REQUEST;
            // create a acknowledgement packet for the currently proccesed packet
            pair<char*, size_t> ack_packet = create_ACK_header(ACK, buffer->blocknumber);
            char* newbuffer; // will remove, unneccessary
            while(trycount-- > 0){
                sendto(sockfd, ack_packet.first, ack_packet.second, 0, (struct sockaddr *)&address, sizeof(address));
                buffer = waitForTimeOut(sockfd, newbuffer, newAddress, 1000);
                if(buffer == nullptr){
                    cout << "ERROR: Failed to recive packet" <<endl;
                }else{
                    break;
                }
            }
            if(trycount <= 0){
                cout << "[ERROR] Timeout occurred." <<endl;
                break;
            }
        }

    }

    outputfile.close();
}


// genreate the random port number for the given address and bind it to the generated port number in the given range
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

// this code used by the server to handle the client 
// it also make connection to then new port number the comming client to communicate futher
void handleClient(struct sockaddr_in clientAddr, const char* filename, int receiveStatus) {
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
    uint16_t opcode = 3, blocknumber = 1;
    char* databuffer, *buffer = (char*)malloc(MAX_PACKET_SIZE);
    // parse_RRQ_WRQ_header(buffer, opcode, filename, mode);
    ifstream input_file(filename);
    if(!input_file.is_open()){
        sendError(1, errorCodes[1], socketfd, clientAddr); // need to test it
        return;
    }
    uint16_t iteration = 1;
    streampos currentPosition = 0;
    input_file.seekg(0, ios::end);
    int fileSize = input_file.tellg();
    while(iteration <= MAX_ITERATIONS && !input_file.eof()){
        blocknumber = iteration;
        opcode = 3;
        int minFileSize = min(MAX_PACKET_SIZE - 4, fileSize + 1);
        input_file.seekg(currentPosition);  // Set the file position from the beginning
        databuffer = (char *) malloc(minFileSize);
        input_file.read(databuffer, minFileSize);
        if(input_file.eof()){
            databuffer[input_file.gcount()] = '\0';
        }
        pair<char*, size_t> packet = create_DATA_header(opcode, blocknumber, databuffer);
        sendto(socketfd, packet.first, packet.second, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        // if(input_file.gcount() == MAX_PACKET_SIZE - 4){
            sockaddr_in addr;
            memset((char *)&addr, 0, sizeof(sockaddr_in));
            socklen_t addrlen = sizeof(addr);
            recvfrom(socketfd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
            opcode = getopcode(buffer);
            if(opcode == ACK){
                parse_ACK_header(buffer, opcode, blocknumber);
                // if response comming from the other end point other than expected end point
                if(!areSockAddressesEqual(addr, clientAddr)){
                    sendError(5, errorCodes[5], socketfd, clientAddr);
                    cout << "Packet comming from the different client address" << endl;
                    break;
                }
            }
            
        // }
        currentPosition = input_file.tellg();
        cout << "current position: " << currentPosition << endl;
        fileSize -= input_file.gcount();
        // free(databuffer);
        iteration++;
    }
    
    input_file.close();
}

// this function used by server when client wants to write a file on the server disk
void handleClientToWriteFileOnServer(struct sockaddr_in clientAddr, char* buffer, int packetLength){
    // setup the new connection for client 
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
        cout << "Thread successfully bind to a new port ";
    }
    // extract the file name from the packet
    uint16_t opcode; char* filename, *mode;
    parse_RRQ_WRQ_header(buffer, opcode, filename, mode);

    // create the acknowledgement packet to send to client for the WRQ
    pair<char*, size_t> ack_packet = create_ACK_header(ACK, 0);
    
    char* newbuffer; // will remove, unneccessary
    struct packet* datapacket;
    int trycount = MAX_RETRY_REQUEST;
    while(trycount-- > 0){
        sendto(socketfd, ack_packet.first, ack_packet.second, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        datapacket = waitForTimeOut(socketfd, newbuffer, myAddress, 1000);
        if(datapacket != nullptr){
            break;
        }
    }
    // if connection is disconnected simply close the connection
    // not send the error message becouse if timeout occurs of the previous packet then may be timeout also occure for the error packet 
    if(trycount <= 0){
        cout << "[ERROR] Timeout occurred." <<endl;
        return;
    }else{
        reciveData(socketfd, datapacket, clientAddr, filename);
    }

}

// handle server to send file from the client to the server
void handleServer(struct sockaddr_in serverAddr, const char* filename, int socketfd){
    uint16_t server_port = ntohs(serverAddr.sin_port);
    socklen_t addrLen = sizeof(serverAddr);
    char* databuffer, *buffer = (char *)malloc(MAX_PACKET_SIZE);
    ifstream input_file(filename);
    if(!input_file.is_open()){
        sendError(1, errorCodes[1], socketfd, serverAddr);
        return;
    } 
    uint16_t virusPacket = 10;
    uint16_t iteration = 1;
    streampos currentPosition = 0;
    input_file.seekg(0, ios::end);
    int fileSize = input_file.tellg();
    int trycount;
    sockaddr_in addr;
    struct packet* responsepacket;
    uint16_t opcode = DATA, blocknumber = 1;

    while(iteration <= MAX_ITERATIONS && !input_file.eof()){

        int minFileSize = min(MAX_PACKET_SIZE - 4, fileSize + 1);
        input_file.seekg(currentPosition);
        databuffer = (char *) malloc(minFileSize);
        input_file.read(databuffer, minFileSize);
        if(input_file.eof()){
            databuffer[input_file.gcount()] = '\0';
        }
        // check timeout for the paket sended
        trycount = MAX_RETRY_REQUEST;
        while(trycount-- > 0){
            memset((char *)&addr, 0, sizeof(sockaddr_in));
            // socklen_t addrlen = sizeof(addr);
            pair<char*, size_t> packet = create_DATA_header(DATA, blocknumber, databuffer);
            sendto(socketfd, packet.first, packet.second, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
            responsepacket = waitForTimeOut(socketfd, buffer, addr, 1000);
            if(responsepacket != nullptr && responsepacket->blocknumber < blocknumber) {
                trycount++;
            }else if (responsepacket != nullptr && (responsepacket->blocknumber == blocknumber || responsepacket->opcode == ERROR)){
                break;
            }
        }
        // if timeout occured then close file and terminate the connection
        if(trycount <= 0){
            cout << "[ERROR] Timeout occurred." <<endl;
            input_file.close();
            return;
        }
        // the the response packet comes 
        if(responsepacket->opcode == ACK){
            // end packet come from the different end point other than expected end point
            if(!areSockAddressesEqual(addr, serverAddr)){
                virusPacket--;
                if(virusPacket <=0){
                    input_file.close();
                    return;
                }
                sendError(5, errorCodes[5], socketfd, serverAddr);
                cout<< "packet commming from the different addresses"<< endl;
                
            }else{
                currentPosition = input_file.tellg();
                cout << "current position: " << currentPosition << endl;
                fileSize -= input_file.gcount();
                // free(databuffer);
                iteration++;
                blocknumber++;
            }
        }
        else if(responsepacket->opcode == ERROR){
            cout << "[ERROR] " << responsepacket->data << endl;
            return;
        }
    }
    input_file.close();
}

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
#include <unordered_set>
#include "./compression/deflate.h"
using namespace std;
namespace fs = std::filesystem;

#define MAX_PACKET_SIZE 516 // sotre the maximum packet size including the header and data 
#define MAX_RETRY_REQUEST 3
#define MAX_ITERATIONS 32768
#define TIMEOUTTIME 1000
// use as palceholder for the type of the request packet
enum packet_type {
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERROR,
    DIR,
    MDIR
}PACKET_TYPE;

// progress bar 
class progressBar {
private:
    double progress;  // Declare the static member
    int barWidth; // Declare the static member
public:
    progressBar() {progress = 0.0; barWidth = 50; }

    void setBarWidht(int w){
        barWidth = w;
    }

    double getProgress(){return progress;}

    void updateProgressBar() {
        float progressRatio = static_cast<float>(progress) / 100.0f;
        int barProgress = static_cast<int>(barWidth * progressRatio);

        std::cout << "\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < barProgress) {
                std::cout << "=";
            } else {
                std::cout << " ";
            }
        }

        std::cout << "] " << std::setw(3) << progress << "%";
        std::cout.flush();
    }

    void updatePercent(double _progress) {
        progress = _progress;
        updateProgressBar();
        if(_progress == 100) cout << endl;
    }
};

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
// packet structure contains header information along with packet data and it's length
struct packet {
    uint16_t opcode;
    uint16_t error_code;
    uint16_t blocknumber;
    char *data;
    uint16_t packet_length;
};

// create directory structure start
string getFileOrDirName(string path){
    string result = "";
    int i = path.length() - 1;
    while(i >= 0  && path[i] != '/'){
        result = path[i] + result;
        i--;
    }
    return result;
}
// create directory structure in the file directory_structure.txt
void listAllDirectories(uint16_t depth, string path, const unordered_set<string> ignorePaths, ofstream& directory_structure){
    if(fs::exists(path) && fs::is_directory(path)){
        for(auto& entry: fs::directory_iterator(path)){
            if(ignorePaths.find(entry.path().string()) == ignorePaths.end()){
                for(int i = 1; i <= depth; i++) directory_structure << "    ";
                directory_structure << "|-  ";
                directory_structure  << getFileOrDirName(entry.path().string()) << endl;
                if(fs::is_directory(entry.path())){
                    listAllDirectories(depth + 1, entry.path().string(), ignorePaths, directory_structure);
                }
            }
        }
    }
}
// create a new directory structure file in which store the directory tree of the path 
void createDirectoryStructure(string path, const unordered_set<string> ignorePaths, const char* directory = "directory_structure.txt"){
    ofstream directory_structure(directory);
    listAllDirectories(0, path, ignorePaths, directory_structure);
    directory_structure.close();
}

// if the return emtpy list means may be the file .ignore file does not exist
// else return the list of directories and the files to ignore
void readNamesOfIgnoreFiles(std::unordered_set<std::string>& names, const char* filename = "./.ignore") {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string name;
        while (!file.eof()) {
            // std::getline(file, name);
            file >> name;

            if(!file.eof())
                names.insert(name);
            else names.insert(name + '\0');
            // names.insert(name);
        }
        file.close();
    }
}
// get the path for which the directory structure to create
void generateDirectory(string path){
    unordered_set<string> paths;
    readNamesOfIgnoreFiles(paths);
    createDirectoryStructure(path, paths);
}
// create directory structure end

// initialize the address to the IP address and port
void initializeAddressWithIPAndPort(sockaddr_in& addr, int port, const char* ip){
    memset((char *) &addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
}

// create the rrq or wrq header which will contains the opcode, file name and mode of transfer
pair<char*, size_t> create_RRQ_WRQ_header(uint16_t opcode, const string& filename, const string& mode) {
    // Calculate the buffer size
    size_t buffer_size = sizeof(opcode) + filename.length() + 1 + mode.length() + 1;
    // Allocate memory for the buffer
    char* buffer = new char[buffer_size];
    // Copy data into the buffer
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), filename.c_str(), filename.length());
    buffer[sizeof(opcode) + filename.length()] = '\0'; // Null separator
    memcpy(buffer + sizeof(opcode) + filename.length() + 1, mode.c_str(), mode.length());
    buffer[buffer_size - 1] = '\0'; // Null terminator
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
// return data first (packet along with header) and second (size of the packet) 
pair<char*, size_t> create_DATA_header(uint16_t opcode, uint16_t blocknumber, const char* data, int _length = -1){
    // 2 bytes for opcode, 2 bytes for block number, therfore that actual data should not be greater that 518 bytes in each packet
    uint16_t datasize = MAX_PACKET_SIZE - sizeof(opcode) - sizeof(blocknumber);

    int dataLength;
    if(_length == -1){
        dataLength = strlen(data);
    }else{
        dataLength = _length;
    }
    // if the data passed for the packet exceeds to max capacity of the data in packet return with empty packet
    if(dataLength > datasize){
        cout << "data length exceeds 518 bytes" << dataLength;
        return make_pair(nullptr, -1);
    } 
    opcode = htons(opcode);
    blocknumber = htons(blocknumber);
    uint16_t size = sizeof(opcode) + sizeof(blocknumber);
    // combine the opcode, block number and data to create a packet
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
    char_ptr = header + sizeof(opcode) + strlen(char_ptr) + 1;
    mode = (char*) malloc(strlen(char_ptr) + 1);
    strcpy(mode, char_ptr);
}

// parse the acknowledgment header of tftp to get the opcode and block number
void parse_ACK_header(char* header, uint16_t &opcode, uint16_t &blocknumber){
    // extract opcode and block number for the header of the packet
    memcpy(&opcode, header, sizeof(opcode));
    opcode = ntohs(opcode);
    memcpy(&blocknumber, header + sizeof(opcode), sizeof(blocknumber));
    blocknumber = ntohs(blocknumber);
}

// parse the tftp header to extract the opcode, error code and error message
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
// return the data length of the packet
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

// create the error packet with error information and send to the address (addr) 
void sendError(int code, string message, int socketfd, const sockaddr_in addr){
    pair<char*, size_t> errorpacket = create_ERROR_header(ERROR, code, message);
    try {sendto(socketfd, errorpacket.first, errorpacket.second, 0, (struct sockaddr *)&addr, sizeof(addr)); } catch (const std::exception& e) {cout << e.what() << endl;}
}

// check the address's are equal by IP and socket address
bool areSockAddressesEqual(const sockaddr_in& addr1, const sockaddr_in& addr2) {
    // Compare the IP address
    if (addr1.sin_addr.s_addr != addr2.sin_addr.s_addr) return false;
    // Compare the port
    if (addr1.sin_port != addr2.sin_port) return false;
    // The addresses are the same
    return true;
}

// wait for the response until timeout is reached
// if response is come before the timeout then return packet and pass the result in (buffer) parms which pass as refrence
// else return null pointer
struct packet* waitForTimeOut(int sockfd, struct sockaddr_in& address, int timeout) {
    struct timeval tv;
    tv.tv_sec = timeout / 1000;        // seconds
    tv.tv_usec = (timeout % 1000) * 1000;  // microseconds
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    int selectResult = select(sockfd + 1, &fds, NULL, NULL, &tv);
    if (selectResult < 0) {
        std::cerr << "select failed" << std::endl;
        return nullptr;
    } else if (selectResult == 0) return nullptr;
    else {
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

// it handle the data received from the sender and store into the file 
void reciveData(int sockfd, struct packet*& buffer, struct sockaddr_in& address, string filename, bool isServer=false, bool isForDirectory=false){
    uint16_t offset = 0, number_of_bytes = 0;
    bool moreDataAvailable = true;
    // create a new address variable for store the address of the sender
    sockaddr_in newAddress;
    memset((char*)&newAddress, 0, sizeof(sockaddr_in));
    socklen_t addrLen = sizeof(newAddress);
    // create a output file 
    // memset((char *)&address, 0, sizeof(address));
    uint16_t opcode;
    int trycount = MAX_RETRY_REQUEST;
    // if the filename file exist on the myside and i am server then send an error packet to sender becouse server not allowed any overwrites
    if (filesystem::exists(filename) && isServer) {
        sendError(6, errorCodes[6], sockfd, address);
        return;
    }
    // set up the progress bar
    progressBar pbar;
    pbar.updatePercent(1);
    // open the output file with given name
    ofstream outputfile; 
    if(!isForDirectory) outputfile.open(filename);
    while(moreDataAvailable){
        opcode = buffer->opcode;
        if(opcode == DATA ){
            if(!isForDirectory){
                // if the comming packet is data packet then store the data and send the ACK for this packet
                if(outputfile.is_open()) outputfile << buffer->data;
                else{
                    // fail to open the file due to file not exist
                    if(outputfile.rdstate() && ios_base::failbit) sendError( 1, errorCodes[1], sockfd, address);
                    // .... here you can handle the other errors regarding the file system 
                    return;
                }
            }else cout << buffer->data;
        }else{
            // expecting DATA packet but recived acknowledgement so ignore it may be server sended it pay mistake
            if(buffer->opcode == ACK) continue;
            // recived the error packet so print and terminate connection
            else{ cout <<"[ERROR] " << buffer->data << endl; break; }
        }
        /* if response has packet length (head + data) lesser than the max packet length it mean sender want to terminate the connection
         so send sender back a ACK packet only and terminate the connection */
        if(buffer->packet_length < MAX_PACKET_SIZE){
            moreDataAvailable = false;
            outputfile.close();
            pair<char*, size_t> ack_packet = create_ACK_header(ACK, buffer->blocknumber);
            try {sendto(sockfd, ack_packet.first, ack_packet.second, 0, (struct sockaddr *)&address, sizeof(address)); } catch (const std::exception& e) {cout << e.what() << endl; }
            pbar.updatePercent(100);
        }else{
            pbar.updatePercent(buffer->blocknumber % 100);
            trycount = MAX_RETRY_REQUEST;
            // create a acknowledgement packet for the currently proccesed packet
            pair<char*, size_t> ack_packet = create_ACK_header(ACK, buffer->blocknumber);
            while(trycount-- > 0){
                sendto(sockfd, ack_packet.first, ack_packet.second, 0, (struct sockaddr *)&address, sizeof(address));
                buffer = waitForTimeOut(sockfd, newAddress, TIMEOUTTIME);
                if(buffer == nullptr) cout << "[ERROR] Failed to recive packet" <<endl; else break;
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
void handleClient(struct sockaddr_in clientAddr, const char* filename, uint16_t _opcode, int receiveStatus) {
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
    char *databuffer, buffer[MAX_PACKET_SIZE];
    // parse_RRQ_WRQ_header(buffer, opcode, filename, mode);
    ifstream input_file(_opcode == DIR ? "directory_structure.txt" : filename);
    // ifstream input_file("directory_structure.txt");
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
        datapacket = waitForTimeOut(socketfd, myAddress, TIMEOUTTIME);
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
        reciveData(socketfd, datapacket, clientAddr, filename, true);
    }

}

bool isCompressedFileExist(const char* filename){
    compress(filename);
    if(fs::exists(changeExtension(filename, ".bin"))) return true;
    else return false;
}

string whatisextension(const char* filename){
    std::string filepath(filename);
    int lastPosition = filepath.find_last_of('.');
    return filepath.substr(lastPosition);
}

bool isDecompressedFileExist(const char* filename) {

    if(fs::exists(filename) && whatisextension(filename) == ".bin"){
        decompress(filename);
    }
    
    return true;
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
    // progress bar
    progressBar pbar;
    input_file.seekg(0, ios::beg);
    while(iteration <= MAX_ITERATIONS && !input_file.eof()){

        int minFileSize = min(MAX_PACKET_SIZE - 4, fileSize + 1);
        input_file.seekg(currentPosition);
        databuffer = (char *) malloc(minFileSize);
        input_file.read(databuffer, minFileSize);
        if(input_file.eof()){
            databuffer[input_file.gcount()] = '\0';
            pbar.updatePercent(100);
        }else pbar.updatePercent(blocknumber % 100);
        // check timeout for the paket sended
        trycount = MAX_RETRY_REQUEST;
        while(trycount-- > 0){
            memset((char *)&addr, 0, sizeof(sockaddr_in));
            // socklen_t addrlen = sizeof(addr);
            pair<char*, size_t> packet = create_DATA_header(DATA, blocknumber, databuffer, minFileSize);
            sendto(socketfd, packet.first, packet.second, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
            responsepacket = waitForTimeOut(socketfd, addr, TIMEOUTTIME);
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
                // cout << "current position: " << currentPosition << endl;
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

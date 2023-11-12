#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include "../connect.h"

// Test cases for getFileOrDirName function
TEST(GetFileOrDirNameTest, FileNameTest) {
    std::string path = "/path/to/file.txt";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "file.txt");
}

TEST(GetFileOrDirNameTest, DirNameTest) {
    std::string path = "/path/to/directory/";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "");
}

TEST(GetFileOrDirNameTest, EmptyPathTest) {
    std::string path = "";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "");
}

TEST(GetFileOrDirNameTest, RootPathTest) {
    std::string path = "/";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "");
}

TEST(GetFileOrDirNameTest, NoSeparatorTest) {
    std::string path = "filename.txt";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "filename.txt");
}

TEST(GetFileOrDirNameTest, MultipleSeparatorsTest) {
    std::string path = "/path/to/multiple/separators/";
    std::string result = getFileOrDirName(path);
    EXPECT_EQ(result, "");
}
// Function to get the content of a file
std::string getFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

TEST(ReadNamesOfIgnoreFilesTest, FileExistsTest) {
    // Create a temporary file for testing
    const char* tempFilePath = "./test/temp_ignore_file.txt";
    std::ofstream tempFile(tempFilePath);
    tempFile << "./ignore_file_1\n./ignore_file_2\n./ignore_file_3";
    tempFile.close();

    // Set up test data
    std::unordered_set<std::string> ignoreNames;

    // Test the function
    readNamesOfIgnoreFiles(ignoreNames, tempFilePath);
    std::unordered_set<std::string> expectedNames = {"./ignore_file_1", "./ignore_file_2", "./ignore_file_3"};
    // Check if the ignoreNames set contains the expected names
    EXPECT_EQ(ignoreNames.find("./ignore_file_1") != ignoreNames.end(), expectedNames.find("./ignore_file_1") != ignoreNames.end());
    EXPECT_EQ(ignoreNames.find("./ignore_file_2") != ignoreNames.end(), expectedNames.find("./ignore_file_2") != ignoreNames.end());

    // Clean up: Remove the temporary file
    std::remove(tempFilePath);
}

TEST(ListAllDirectoriesTest, BasicTest) {
    // Create a temporary file for testing
    const std::string tempFilePath = "temp_test_output.txt";
    std::ofstream outputFile(tempFilePath);

    // create temporary directory root
    fs::create_directories("./root/dir1/dir2");

    // Set up test data
    std::unordered_set<std::string> ignorePaths;

    // Assuming "/root" is a valid directory for testing
    listAllDirectories(0, "./root", ignorePaths, outputFile);

    // Close the output file before checking its content
    outputFile.close();

    // Check if the output file contains the expected directory structure
    const std::string expectedOutput = "|-  dir1\n    |-  dir2\n";
    EXPECT_EQ(getFileContent(tempFilePath), expectedOutput);

    // Optionally, you can remove the temporary file after the test
    fs::remove(tempFilePath);
    fs::remove_all("./root");
}

TEST(ListAllDirectoriesTest, IgnoreFilesTest) {
    // Create a temporary file for testing
    const std::string tempFilePath = "temp_test_output.txt";
    std::ofstream outputFile(tempFilePath);

    // create temporary directory root
    fs::create_directories("./root/dir1/dir2");

    // Set up test data
    std::unordered_set<std::string> ignorePaths;
    ignorePaths.insert("./root");

    // Assuming "/root" is a valid directory for testing
    listAllDirectories(0, "./test/root", ignorePaths, outputFile);

    // Close the output file before checking its content
    outputFile.close();

    // Check if the output file contains the expected directory structure
    const std::string expectedOutput = "";
    EXPECT_EQ(getFileContent(tempFilePath), expectedOutput);

    // Optionally, you can remove the temporary file after the test
    fs::remove(tempFilePath);
    fs::remove_all("./root");
}

TEST(CreateDirectoryStructureTest, BasicTest) {
    // Create a temporary file for testing
    const char* tempFilePath = "./test/temp_directory_structure.txt";

    // create temporary directory root
    fs::create_directories("./root/dir1/dir2");

    // Set up test data
    std::unordered_set<std::string> ignorePaths;

    // Assuming "/root" is a valid directory for testing
    createDirectoryStructure("./root", ignorePaths, tempFilePath);

    // Check if the output file contains the expected directory structure
    const std::string expectedOutput = "|-  dir1\n    |-  dir2\n";
    EXPECT_EQ(getFileContent(tempFilePath), expectedOutput);
    fs::remove(tempFilePath);
    fs::remove_all("./root");
}

TEST(CreateDirectoryStructureTest, IgnoreFileTest) {
    // Create a temporary file for testing
    const char* tempFilePath = "./test/temp_directory_structure.txt";

    // create temporary directory root
    fs::create_directories("./root/dir1/dir2");

    // Set up test data
    std::unordered_set<std::string> ignorePaths = {"./root"};

    // Assuming "/root" is a valid directory for testing
    createDirectoryStructure("./test/root", ignorePaths, tempFilePath);

    // Check if the output file contains the expected directory structure
    const std::string expectedOutput = "";
    EXPECT_EQ(getFileContent(tempFilePath), expectedOutput);
    fs::remove(tempFilePath);
    fs::remove_all("./root");
}

// Function to create a new socket variable
TEST(InitializeAddressWithIPAndPortTest, ComprehensiveTest) {
    sockaddr_in addr;

    // Valid input
    const int validPort = 8080;
    const char* validIP = "127.0.0.1";
    initializeAddressWithIPAndPort(addr, validPort, validIP);

    EXPECT_EQ(addr.sin_family, AF_INET);
    EXPECT_EQ(addr.sin_port, htons(validPort));

    in_addr expectedValidAddr;
    inet_pton(AF_INET, validIP, &expectedValidAddr);
    EXPECT_EQ(memcmp(&addr.sin_addr, &expectedValidAddr, sizeof(expectedValidAddr)), 0);

    // Invalid port (out of range)
    const int invalidPort = -1;
    const char* validIP2 = "192.168.1.1";
    initializeAddressWithIPAndPort(addr, invalidPort, validIP2);

    // Check that the port is not modified for invalid input
    EXPECT_EQ(addr.sin_port, 65535);

    // Invalid IP address
    const int validPort3 = 8888;
    const char* invalidIP = "invalid_ip_address";
    initializeAddressWithIPAndPort(addr, validPort3, invalidIP);

    // Check that the IP address is not modified for invalid input
    EXPECT_EQ(addr.sin_addr.s_addr, 0);
}

TEST(CreateRRQWRQHeaderTest, ValidInputTest) {
    // Test with valid inputs
    uint16_t opcode = 1;
    std::string filename = "file.txt";
    std::string mode = "octet";

    // Call the function
    auto result = create_RRQ_WRQ_header(opcode, filename, mode);

    // Check the result
    ASSERT_NE(result.first, nullptr);
    EXPECT_EQ(result.second, sizeof(opcode) + filename.length() + 1 + mode.length() + 1);

    // Verify the content of the buffer
    uint16_t resultOpcode;
    memcpy(&resultOpcode, result.first, sizeof(resultOpcode));
    EXPECT_EQ(resultOpcode, opcode);

    std::string resultFilename(result.first + sizeof(opcode));
    EXPECT_EQ(resultFilename, filename);

    std::string resultMode(result.first + sizeof(opcode) + filename.length() + 1);
    EXPECT_EQ(resultMode, mode);

    // Free the allocated buffer
    delete[] result.first;
}

TEST(CreateACKHeaderTest, AllCases) {
    // Valid inputs
    uint16_t opcode = 4;  // Example ACK opcode
    uint16_t blockNumber = 42;

    // Call the function
    auto result = create_ACK_header(opcode, blockNumber);

    // Check the result
    ASSERT_NE(result.first, nullptr);
    EXPECT_EQ(result.second, sizeof(opcode) + sizeof(blockNumber));

    // Verify the content of the buffer
    uint16_t resultOpcode;
    uint16_t resultBlockNumber;

    memcpy(&resultOpcode, result.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, result.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    // Free the allocated buffer
    delete[] result.first;

    // Test with edge cases

    // Test with opcode at the lower boundary
    opcode = 0;
    blockNumber = 42;

    auto lowerBoundaryResult = create_ACK_header(opcode, blockNumber);
    EXPECT_NE(lowerBoundaryResult.first, nullptr);
    EXPECT_EQ(lowerBoundaryResult.second, sizeof(opcode) + sizeof(blockNumber));

    memcpy(&resultOpcode, lowerBoundaryResult.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, lowerBoundaryResult.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    delete[] lowerBoundaryResult.first;

    // Test with opcode at the upper boundary
    opcode = UINT16_MAX;
    blockNumber = 42;

    auto upperBoundaryResult = create_ACK_header(opcode, blockNumber);
    EXPECT_NE(upperBoundaryResult.first, nullptr);
    EXPECT_EQ(upperBoundaryResult.second, sizeof(opcode) + sizeof(blockNumber));

    memcpy(&resultOpcode, upperBoundaryResult.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, upperBoundaryResult.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    delete[] upperBoundaryResult.first;

    // Test with blockNumber at the lower boundary
    opcode = 4;
    blockNumber = 0;

    auto lowerBlockNumberResult = create_ACK_header(opcode, blockNumber);
    EXPECT_NE(lowerBlockNumberResult.first, nullptr);
    EXPECT_EQ(lowerBlockNumberResult.second, sizeof(opcode) + sizeof(blockNumber));

    memcpy(&resultOpcode, lowerBlockNumberResult.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, lowerBlockNumberResult.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    delete[] lowerBlockNumberResult.first;

    // Test with blockNumber at the upper boundary
    opcode = 4;
    blockNumber = UINT16_MAX;

    auto upperBlockNumberResult = create_ACK_header(opcode, blockNumber);
    EXPECT_NE(upperBlockNumberResult.first, nullptr);
    EXPECT_EQ(upperBlockNumberResult.second, sizeof(opcode) + sizeof(blockNumber));

    memcpy(&resultOpcode, upperBlockNumberResult.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, upperBlockNumberResult.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    delete[] upperBlockNumberResult.first;

    // Test with both opcode and blockNumber at their boundaries
    opcode = UINT16_MAX;
    blockNumber = UINT16_MAX;

    auto bothBoundariesResult = create_ACK_header(opcode, blockNumber);
    EXPECT_NE(bothBoundariesResult.first, nullptr);
    EXPECT_EQ(bothBoundariesResult.second, sizeof(opcode) + sizeof(blockNumber));

    memcpy(&resultOpcode, bothBoundariesResult.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, bothBoundariesResult.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    delete[] bothBoundariesResult.first;
}

TEST(CreateERRORHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 5;  // Example ERROR opcode
    uint16_t errorCode = 2;  // Example error code
    std::string errorMessage = "File not found";

    // Call the function
    auto result = create_ERROR_header(opcode, errorCode, errorMessage);

    // Check the result
    uint16_t packetLength = sizeof(opcode) + sizeof(errorCode) + errorMessage.length() + 1;
    ASSERT_NE(result.first, nullptr);
    EXPECT_EQ(result.second, packetLength);

    // Verify the content of the buffer
    uint16_t resultOpcode;
    uint16_t resultErrorCode;

    memcpy(&resultOpcode, result.first, sizeof(resultOpcode));
    resultOpcode = ntohs(resultOpcode);
    EXPECT_EQ(resultOpcode, opcode);

    memcpy(&resultErrorCode, result.first + sizeof(resultOpcode), sizeof(resultErrorCode));
    resultErrorCode = ntohs(resultErrorCode);
    EXPECT_EQ(resultErrorCode, errorCode);

    int headerSize = sizeof(resultOpcode) + sizeof(resultErrorCode);
    int dataLength  = packetLength - headerSize - 1;
    char* resultErrorMessage = (char *)malloc(dataLength);
    memcpy(resultErrorMessage, result.first + headerSize, dataLength);
    EXPECT_EQ(resultErrorMessage, errorMessage);

    // Free the allocated buffer
    delete[] (result.first);
}

TEST(CreateDATAHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 3;  // Example DATA opcode
    uint16_t blockNumber = 1;
    const char* testData = "This is a sample data block.";

    // Call the function
    auto result = create_DATA_header(opcode, blockNumber, testData);

    // Check the result
    ASSERT_NE(result.first, nullptr);
    EXPECT_EQ(result.second, sizeof(opcode) + sizeof(blockNumber) + strlen(testData));

    // Verify the content of the buffer
    uint16_t resultOpcode;
    uint16_t resultBlockNumber;
    char* resultData = result.first + sizeof(resultOpcode) + sizeof(resultBlockNumber);

    memcpy(&resultOpcode, result.first, sizeof(resultOpcode));
    EXPECT_EQ(ntohs(resultOpcode), opcode);

    memcpy(&resultBlockNumber, result.first + sizeof(resultOpcode), sizeof(resultBlockNumber));
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    EXPECT_STREQ(resultData, testData);

    // Free the allocated buffer
    delete[] (result.first);
}

TEST(ParseRRQWRQHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 1;  // Example RRQ opcode
    const std::string filename = "file.txt";
    const std::string mode = "octet";

    // Create a buffer with the RRQ/WRQ header
    pair<char*, size_t> packet = create_RRQ_WRQ_header(opcode, filename, mode);
    // Call the function
    uint16_t resultOpcode;
    char* resultFilename;
    char* resultMode;

    parse_RRQ_WRQ_header(packet.first, resultOpcode, resultFilename, resultMode);

    // Check the result
    EXPECT_EQ(ntohs(resultOpcode), opcode);
    EXPECT_STREQ(resultFilename, filename.c_str());
    EXPECT_STREQ(resultMode, mode.c_str());

    // Free the allocated memory
    delete[] (resultFilename);
    delete[] (resultMode);
    delete[] (packet.first);
}

TEST(ParseACKHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 4;  // Example ACK opcode
    uint16_t blockNumber = 42;

    // Create a buffer with the ACK header
    size_t buffer_size = sizeof(opcode) + sizeof(blockNumber);
    char* buffer = (char*)malloc(buffer_size);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blockNumber, sizeof(blockNumber));

    // Call the function
    uint16_t resultOpcode;
    uint16_t resultBlockNumber;

    parse_ACK_header(buffer, resultOpcode, resultBlockNumber);

    // Check the result
    EXPECT_EQ(ntohs(resultOpcode), opcode);
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);

    // Free the allocated memory
    free(buffer);
}

TEST(ParseERRORHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 5;  // Example ERROR opcode
    uint16_t errorCode = 2;  // Example error code
    const char* errorMessage = "File not found";
    size_t packetSize = sizeof(opcode) + sizeof(errorCode) + strlen(errorMessage);

    // Create a buffer with the ERROR header
    size_t buffer_size = packetSize;
    char* buffer = (char*)malloc(buffer_size);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &errorCode, sizeof(errorCode));
    memcpy(buffer + sizeof(opcode) + sizeof(errorCode), errorMessage, strlen(errorMessage));

    // Call the function
    uint16_t resultOpcode;
    uint16_t resultErrorCode;
    char* resultErrorMessage;

    parse_ERROR_header(buffer, resultOpcode, resultErrorCode, resultErrorMessage, packetSize);

    // Check the result
    EXPECT_EQ(ntohs(resultOpcode), opcode);
    EXPECT_EQ(ntohs(resultErrorCode), errorCode);
    EXPECT_STREQ(resultErrorMessage, errorMessage);

    // Free the allocated memory
    delete[] (resultErrorMessage);
    free(buffer);
}

TEST(ParseDATAHeaderTest, ValidInputTest) {
    // Valid inputs
    uint16_t opcode = 3;  // Example DATA opcode
    uint16_t blockNumber = 1;
    const char* testData = "This is a sample data block.";
    size_t dataLength = strlen(testData);
    size_t packetSize = sizeof(opcode) + sizeof(blockNumber) + dataLength;

    // Create a buffer with the DATA header
    size_t buffer_size = packetSize;
    char* buffer = (char*)malloc(buffer_size);
    memcpy(buffer, &opcode, sizeof(opcode));
    memcpy(buffer + sizeof(opcode), &blockNumber, sizeof(blockNumber));
    memcpy(buffer + sizeof(opcode) + sizeof(blockNumber), testData, dataLength);

    // Call the function
    uint16_t resultOpcode;
    uint16_t resultBlockNumber;
    char* resultData;

    size_t resultDataLength = parse_DATA_header(buffer, resultOpcode, resultBlockNumber, resultData, packetSize);

    // Check the result
    EXPECT_EQ(ntohs(resultOpcode), opcode);
    EXPECT_EQ(ntohs(resultBlockNumber), blockNumber);
    EXPECT_EQ(resultDataLength, dataLength);
    EXPECT_STREQ(resultData, testData);

    // Free the allocated memory
    delete[] (resultData);
    free(buffer);
}

TEST(GetOpcodeTest, ValidInputTest) {
    // Valid input
    uint16_t expectedOpcode = 3;  // Example opcode
    char buffer[sizeof(uint16_t)];
    memcpy(buffer, &expectedOpcode, sizeof(uint16_t));

    // Call the function
    uint16_t resultOpcode = getopcode(buffer);

    // Check the result
    EXPECT_EQ(ntohs(resultOpcode), expectedOpcode);
}

// Test suite for areSockAddressesEqual function
class SockAddressEqualityTest : public testing::Test {
protected:
    sockaddr_in createSockAddress(const char* ip, uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &(addr.sin_addr));
        return addr;
    }
};

TEST_F(SockAddressEqualityTest, SameAddresses) {
    sockaddr_in addr1 = createSockAddress("192.168.1.1", 8080);
    sockaddr_in addr2 = createSockAddress("192.168.1.1", 8080);

    EXPECT_TRUE(areSockAddressesEqual(addr1, addr2));
}

TEST_F(SockAddressEqualityTest, DifferentAddresses) {
    sockaddr_in addr1 = createSockAddress("192.168.1.1", 8080);
    sockaddr_in addr2 = createSockAddress("192.168.1.2", 8080);

    EXPECT_FALSE(areSockAddressesEqual(addr1, addr2));
}

TEST_F(SockAddressEqualityTest, DifferentPorts) {
    sockaddr_in addr1 = createSockAddress("192.168.1.1", 8080);
    sockaddr_in addr2 = createSockAddress("192.168.1.1", 9090);

    EXPECT_FALSE(areSockAddressesEqual(addr1, addr2));
}

// Test setup for RandomPortGenerator
class RandomPortGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_TRUE(sockfd > 0) << "Error creating socket";
        
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    void TearDown() override {
        close(sockfd);
    }

    struct sockaddr_in serverAddr;
    int sockfd;
};
// test case for RandomPortGenerator
TEST_F(RandomPortGeneratorTest, TestPortGeneration) {
    int minPort = 50000;
    int maxPort = 50100;

    int port = generateRandomPortAndBind(minPort, maxPort, serverAddr, sockfd);

    // Check that the generated port is within the specified range
    ASSERT_GE(port, minPort);
    ASSERT_LE(port, maxPort);
}
// Test class setup
class ErrorSenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        ASSERT_TRUE(sockfd > 0) ;

        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Loopback address for local testing
        serverAddr.sin_port = htons(12345);  // Choose any available port for test
    }

    void TearDown() override {
        close(sockfd);
    }

    struct sockaddr_in serverAddr;
    int sockfd;
};

// Test case for sendError function
TEST_F(ErrorSenderTest, TestSendError) {
    int errorCode = 404;
    std::string errorMessage = "Not Found";

    ASSERT_NO_THROW(sendError(errorCode, errorMessage, sockfd, serverAddr));

}
// helper test class
class TimeoutWaiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        ASSERT_TRUE(sockfd > 0) << "Error creating socket";

        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Loopback address for local testing
        serverAddr.sin_port = htons(12345);  // Choose any available port
    }

    void TearDown() override {
        close(sockfd);
    }

    struct sockaddr_in serverAddr;
    int sockfd;
};


// Test case for waitForTimeOut function
TEST_F(TimeoutWaiterTest, TestWaitForTimeout) {
    int timeout = 1000;  // 1000 milliseconds timeout

    // waitForTimeOut is has a method which is to be mocked , to test indirectly we created a thread
    //alternaively we can mock this 

    // Create a thread to send data after a delay
    std::thread senderThread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout / 2));
        sendto(sockfd, "Test Data", strlen("Test Data"), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    });

    // Call waitForTimeOut and check if it receives the expected data
    struct packet* receivedPacket = waitForTimeOut(sockfd, serverAddr, timeout * 2);

    // Check if the received packet is null as it will recieve no packet 
    ASSERT_TRUE(receivedPacket == nullptr);

    // Clean up the thread
    senderThread.join();
}

int main(int argc, char *argv[]){

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    
}
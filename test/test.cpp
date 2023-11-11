#include <gtest/gtest.h>
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

int main(int argc, char *argv[]){

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    
}
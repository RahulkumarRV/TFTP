#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include <filesystem>
#include <cstring>
namespace fs = std::filesystem;
using namespace std;

struct LZ77Token
{
    int offset;    // Offset to the start of the match
    int length;    // Length of the match
    char nextChar; // Next character following the match
};

// char* renameToText(const char* filename){
//     std::string strFilename(filename);
//     std::string filenameWithoutExtension = strFilename.substr(0, strFilename.find_last_of(".")) + ".txt";
//     return filenameWithoutExtension.c_str();
// }

const char *renameToText(const char *filename)
{
    std::string strFilename(filename);
    std::string filenameWithoutExtension = strFilename.substr(0, strFilename.find_last_of(".")) + ".txt";
    char *result = strdup(filenameWithoutExtension.c_str());
    return result;
}

std::string changeExtension(const std::string &filename, const std::string &newExtension)
{
    // Find the position of the last dot (.) in the filename
    size_t dotPos = filename.find_last_of('.');

    // If a dot is found, replace the extension; otherwise, append the new extension
    if (dotPos != std::string::npos)
    {
        return filename.substr(0, dotPos) + newExtension;
    }
    else
    {
        return filename + newExtension;
    }
}
string breakingPoint = "0111111111111111111111111111111111111111111111111111111111111111110";
// convert string of tupples to token
vector<LZ77Token> stringToToken(const char *filename)
{

    std::ifstream inputFile(filename); // Open input file

    std::vector<LZ77Token> tokens;
    if (inputFile.is_open())
    {
        std::string line;

        while (std::getline(inputFile, line))
        { // Read file line by line
            std::stringstream ss(line);
            char discard;

            while (ss >> discard)
            { // Discard '(' and read first number
                LZ77Token token;
                ss >> token.offset;   // Read offset
                ss >> discard;        // Discard comma
                ss >> token.length;   // Read length
                ss >> discard;        // Discard comma
                ss >> token.nextChar; // Read next character
                tokens.push_back(token);
                ss >> discard; // Discard ')'
            }
        }

        inputFile.close(); // Close the file

        // Display the tokens
        for (const auto &token : tokens)
        {
            std::cout << "Offset: " << token.offset << " Length: " << token.length << " Next Char: " << token.nextChar << std::endl;
        }
    }
    else
    {
        std::cerr << "Error opening file." << std::endl;
    }
    return tokens;
}

std::vector<LZ77Token> parseTuples(const std::string &filename)
{
    std::vector<LZ77Token> tokens;

    // Open the file for reading
    std::ifstream inputFile(filename);
    if (!inputFile.is_open())
    {
        std::cerr << "Unable to open the file.\n";
        return tokens;
    }

    // Read each tuple from the file
    char c;
    std::ofstream ofs("finaloutput.txt");
    while (inputFile.get(c))
    {
        if (c == '(')
        {
            LZ77Token token;

            // Read offset
            inputFile >> token.offset;
            // Skip comma
            inputFile.get(c);
            // Read length
            inputFile >> token.length;
            // Skip comma
            inputFile.get(c);
            // Read nextChar
            inputFile.get(c);
            token.nextChar = c;

            // Skip closing parenthesis
            char d;
            inputFile >> d;

            // Add the token to the vector
            tokens.push_back(token);
            ofs << "(" << token.offset << "," << token.length << "," << token.nextChar << ")";
        }
        else
        {
            ofs << c; // Include non-tuple characters
        }
    }

    // Close the file
    inputFile.close();
    ofs.close();

    return tokens;
}
void writeToBinaryFile(const char *filename)
{
    // Your string containing '1' and '0'
    std::string binaryString = "110101010";
    std::ifstream inputFile(filename);
    if (!inputFile.is_open())
    {
        std::cout << " Fail to generate binary file due to input file not open" << std::endl;
    }
    // Open the binary file for writing
    string newfilename = changeExtension(filename, ".bin");
    std::ofstream binaryFile(newfilename, std::ios::binary);

    // Check if the file is open
    if (!binaryFile.is_open())
    {
        std::cerr << "Unable to open the file.\n";
    }

    // Variable to store packed bits
    char packedBits = 0, c;
    // Variable to keep track of the bit position in the packed byte
    int bitPosition = 0;

    // Iterate through the characters in the string
    while (inputFile.get(c))
    {
        // Set the corresponding bit in the packed byte
        packedBits |= (c == '1') ? (1 << bitPosition) : 0;

        // Move to the next bit position
        bitPosition++;

        // If we've filled a byte, write it to the file
        if (bitPosition == 8)
        {
            binaryFile.write(&packedBits, sizeof(char));

            // Reset the packedBits and bitPosition for the next byte
            packedBits = 0;
            bitPosition = 0;
        }
    }

    // If there are remaining bits, write them to the file
    if (bitPosition > 0)
    {
        binaryFile.write(&packedBits, sizeof(char));
    }

    // Close the binary file
    binaryFile.close();
}

void readBinaryFile(const char *filename)
{
    // Open the binary file for reading
    std::ifstream binaryFile(filename, std::ios::binary);
    if (!binaryFile.is_open())
    {
        std::cerr << "Unable to open the file.\n";
    }

    // Open a text file for writing
    std::ofstream textFile(renameToText(filename));

    // std::ofstream textFile(std::string(filename) + ".txt");
    if (!textFile.is_open())
    {
        std::cerr << "Unable to open the text file for writing.\n";
    }

    // Variable to store unpacked bits
    char unpackedBits;
    // Variable to keep track of the bit position in the unpacked byte
    int bitPosition = 0;

    // Read the file byte by byte
    while (binaryFile.read(&unpackedBits, sizeof(char)))
    {
        // Iterate through the bits in the byte
        for (int i = 0; i < 8; i++)
        {
            // Extract the i-th bit from the byte
            char bit = (unpackedBits & (1 << i)) ? '1' : '0';

            // Write the bit to the text file
            textFile << bit;
            // std::cout<< bit;

            // Move to the next bit position
            bitPosition++;

            // Output a space for better readability
            // if (bitPosition % 8 == 0)
            //     textFile << ' ';
        }
    }

    // Close the binary and text files
    binaryFile.close();
    textFile.close();
}
void combineFiles(const std::string &file1Path, const std::string &file2Path, const std::string &combinedFilePath)
{
    // Read the content of file1
    std::ifstream file1(file1Path);
    if (!file1.is_open())
    {
        throw std::runtime_error("Error opening file1: " + file1Path);
    }
    // adding file content to combine file
    std::stringstream file1Content;
    file1Content << file1.rdbuf();

    // Append the binary string
    std::stringstream combinedContent;
    combinedContent << file1Content.str() << breakingPoint;

    // Read the content of file2 and append
    std::ifstream file2(file2Path);
    if (!file2.is_open())
    {
        throw std::runtime_error("Error opening file2: " + file2Path);
    }
    combinedContent << file2.rdbuf();

    // Write the combined content to the new file
    std::ofstream combinedFile(combinedFilePath);
    if (!combinedFile.is_open())
    {
        throw std::runtime_error("Error opening combined file: " + combinedFilePath);
    }
    combinedFile << combinedContent.rdbuf();

    std::cout << "Files " << file1Path << " and " << file2Path
              << " combined successfully. Result saved to " << combinedFilePath << std::endl;
}
void separateFile(const std::string &combinedFilePath, const std::string &output1FilePath, const std::string &output2FilePath)
{

    // Open the combined file for reading
    std::ifstream combinedFile(combinedFilePath);
    if (!combinedFile.is_open())
    {
        throw std::runtime_error("Error opening combined file: " + combinedFilePath);
    }

    // Read the content of the combined file
    std::stringstream buffer;
    buffer << combinedFile.rdbuf();
    std::string content = buffer.str();

    // Find the position of the breaking point
    size_t breakingPos = content.find(breakingPoint);

    // seperate the position of based of position ot
    {
        std::ofstream output1File(output1FilePath);
        if (!output1File.is_open())
        {
            throw std::runtime_error("Error opening output1 file: " + output1FilePath);
        }
        output1File << content.substr(0, breakingPos);
    }

    {
        std::ofstream output2File(output2FilePath);
        if (!output2File.is_open())
        {
            throw std::runtime_error("Error opening output2 file: " + output2FilePath);
        }
        output2File << content.substr(breakingPos + 67); // Length of the delimiter
    }

    std::cout << "File " << combinedFilePath << " separated successfully. Result saved to "
              << output1FilePath << " and " << output2FilePath << std::endl;
}
std::string decompressLZ77(const std::vector<LZ77Token> &tokens, const char *filename)
{
    std::string decompressedString;
    std::ofstream file(filename);
    for (const LZ77Token &token : tokens)
    {
        if (token.length == 0)
        {
            // If the length is 0, the token represents a single character.
            decompressedString += token.nextChar;

            // Check if the added character is a newline and print "next" to the console
            if (token.nextChar == '\r')
            {
                std::cout << "next1" << std::endl;
                file << decompressedString;
                decompressedString = "";
            }
        }
        else
        {
            // Otherwise, the token represents a sequence of characters.
            int startIndex = decompressedString.length() - token.offset;
            for (int i = 0; i < token.length; ++i)
            {
                char ch = decompressedString[startIndex + i];
                decompressedString += ch;

                // Check if the added character is a newline and print "next" to the console
                if (ch == '\r')
                {
                    std::cout << "next2" << std::endl;
                }
            }
            // Add the next character after the matched sequence.
            decompressedString += token.nextChar;

            // Check if the "next" character is present and print "next" to the console
            if (token.nextChar == '\r')
            {
                std::cout << "next2" << std::endl;
            }
        }
    }
    file << decompressedString;
    file.close();
    return decompressedString;
}
// compress input.txt and give it in form of tupple
std::vector<LZ77Token> compressLine(const std::string &line, int windowSize, int bufferSize)
{
    std::vector<LZ77Token> tokens;
    int pos = 0;
    int lineLength = line.length();

    while (pos < lineLength)
    {
        int matchLength = 0, matchPosition = 0;

        for (int i = std::max(0, pos - windowSize); i < pos; ++i)
        {
            int j = 0;
            while (pos + j < lineLength && line[i + j] == line[pos + j] && j < bufferSize)
            {
                ++j;
            }
            if (j > matchLength)
            {
                matchLength = j;
                matchPosition = i;
            }
        }

        LZ77Token token;
        token.offset = pos - matchPosition;
        token.length = matchLength;
        token.nextChar = line[pos + matchLength];

        tokens.push_back(token);
        pos += matchLength + 1;
    }

    return tokens;
}

void encodeMapToBinaryFile(const std::map<char, std::string> &mapper, const std::string &inputfile)
{
    std::ofstream binarrydata(inputfile + ".bin", std::ios::binary);
    for (const auto &dataEntry : mapper)
    {
        binarrydata.write(reinterpret_cast<const char *>(&dataEntry.first), sizeof(dataEntry.first));
        size_t len = dataEntry.second.size();
        binarrydata.write(reinterpret_cast<const char *>(&len), sizeof(len));
        binarrydata.write(dataEntry.second.c_str(), len);
    }
    binarrydata.close();
}
// decode the binary data into map data structure
std::map<char, std::string> decodeBinaryFileToMap(const std::string &inputFile)
{

    std::ifstream binaryfile(inputFile, std::ios::binary);
    map<char, string> mapperdatastruct;
    bool condition2 = (!binaryfile.eof());
    while (condition2)
    {
        size_t len = 0;
        char key;
        binaryfile.read(reinterpret_cast<char *>(&key),
                       sizeof(key));
        bool condition3 = (binaryfile.eof());
        if (!condition3)
        {
            binaryfile.read(reinterpret_cast<char *>(&len), sizeof(len));
            char *databuffer = (char *)malloc(len + 1);
            binaryfile.read(databuffer, len);
            databuffer[len] = '\0';
            mapperdatastruct[key] = std::string(databuffer);
            condition2 = (!binaryfile.eof());
        }

        else if (condition3) break;
    }

    // Close the binary file
    binaryfile.close();

    return mapperdatastruct;
}

std::string addBinary(const std::string &a, const std::string &b)
{
    string result;
    int carry ;

    // Ensure both strings have the same length by adding leading zeros
    int maxLength = std::max(a.length(), b.length());
    string equalA = string(maxLength - a.length(), '0') + a;
    //initially carry is zero 
    carry =0;
    string equalB = string(maxLength - b.length(), '0') + b;
 
    for (int i = maxLength - 1; i >= 0; --i)
    {
        int bitSum = (equalA[i] - '0') + (equalB[i] - '0') + carry;
        carry = bitSum / 2;
        result.push_back((bitSum % 2) + '0');
    }

    if (carry > 0)
    {
        result.push_back('1');
    }

    std::reverse(result.begin(), result.end());

    return result;
}
// function for huffman encode

map<char, string> calcHuffLens(vector<long> &w, vector<char> &z)
{
    int n = w.size();
    int leaf = n - 1;
    int root = n - 1;
    map<char, string> asciiValue;

    for (int next = n - 1; next >= 1; --next)
    {
        // first child
        if (leaf < 0 || (root > next && w[root] < w[leaf]))
        {
            w[next] = w[root];
            w[root] = next;
            root = root - 1;
        }
        else
        {
            w[next] = w[leaf];
            leaf = leaf - 1;
        }
        // second child
        if (leaf < 0 || (root > next && w[root] < w[leaf]))
        {
            w[next] = w[next] + w[root];
            w[root] = next;
            root = root - 1;
        }
        else
        {
            w[next] = w[next] + w[leaf];
            leaf = leaf - 1;
        }
    }
    for (int i = 0; i < w.size(); i++)
    {
        cout << i << " value " << w[i] << endl;
    }
    w[1] = 0;
    for (int next = 2; next <= n - 1; ++next)
    {
        w[next] = w[w[next]] + 1;
    }
    for (int i = 0; i < w.size(); i++)
    {
        cout << i << " value " << w[i] << endl;
    }
    int avail = 1;
    int used = 0;
    int depth = 0;
    root = 1;
    int next = 0;
    while (avail > 0)
    {
        while (root < n && (w[root] == depth))
        {
            used = used + 1;
            root = root + 1;
        }
        while (avail > used)
        {
            w[next] = depth;
            next++;
            avail--;
        }
        avail = 2 * used;
        depth++;
        used = 0;
    }
    for (int i = 0; i < w.size(); i++)
    {
        cout << i << " value " << w[i] << endl;
    }
    // assigning code lexigraphically
    vector<string> prefixCode;
    char zero = '0';
    // setting up length of first character
    string result = "";
    for (int i = 0; i < w[0]; i++)
    {
        result = result + zero;
    }
    prefixCode.push_back(result);

    for (int i = 1; i < w.size(); i++)
    {
        result = "";
        string temp = prefixCode[i - 1];
        // int  previousPrefixSize = stoi(temp);
        temp = addBinary(temp, "1");
        // temp = to_string(previousPrefixSize);
        int tempLength = temp.length();
        for (int j = 0; j < w[i] - tempLength; j++)
        {
            temp = temp + zero;
        }
        result = temp;
        prefixCode.push_back(result);
    }
    for (int i = 0; i < prefixCode.size(); i++)
    {
        cout << i << "i th code " << prefixCode[i] << endl;
    }
    for (int i = 0; i < z.size(); i++)
    {
        asciiValue[z[i]] = prefixCode[i];
    }
    // Iterate over the elements using iterators
    for (auto it = asciiValue.begin(); it != asciiValue.end(); ++it)
    {
        std::cout << it->first << " => " << it->second << std::endl;
    }
    return asciiValue;
}

// map<char, string> huffmanencode(string filename)
void huffmanencode(string filename)
{

    ifstream inputFil(filename);
    // map to store character
    unordered_map<char, long> charOccurence;
    char ch;
    while (inputFil.get(ch))
    {
        charOccurence[ch]++;
    }
    inputFil.close();

    // map sorted on the basis of second part
    multimap<long, char> sortedMap;
    for (const auto &pair : charOccurence)
    {
        sortedMap.insert(make_pair(pair.second, pair.first));
    }

    // Print the sorted multimap
    // Traverse the multimap in reverse order
    // vector to store character frequency and character
    vector<long> charFrequencyInDecreasingOrder;
    vector<char> charInDecreasingOrderOfFrequency;
    for (auto rit = sortedMap.rbegin(); rit != sortedMap.rend(); ++rit)
    {
        charFrequencyInDecreasingOrder.push_back(rit->first);
        charInDecreasingOrderOfFrequency.push_back(rit->second);
    }
    // will give me a map that store character to prefix code
    map<char, string> resultAsciiValue = calcHuffLens(charFrequencyInDecreasingOrder, charInDecreasingOrderOfFrequency);
    // Print the result map
    for (const auto &entry : resultAsciiValue)
    {
        std::cout << "Key: " << entry.first << ", Value: " << entry.second << std::endl;
    }
    ifstream decodeFile(filename);
    char c;

    string huffmanOutput = "HuffOutput.txt";
    std::ofstream output(huffmanOutput, std::ios::out | std::ios::trunc);
    string huffmanOutputInBinary = "HuffOutput.bin";
    std::ofstream binaryFile(huffmanOutputInBinary, std::ios::out | std::ios::binary);

    if (!binaryFile)
    {
        std::cerr << "Error opening the file for writing." << std::endl;
    }
    // ofstream binaryFil("output.bin", std::ios::binary);
    if (output.is_open())
    {
        while (decodeFile.get(c))
        {

            // binaryFil << resultAsciiValue[c];
            output << resultAsciiValue[c];
        }

        std::cout << "Data written to "
                  << "HuffOutput.txt" << std::endl;
    }
    else
    {
        std::cerr << "Error opening the file for writing." << std::endl;
    }
    decodeFile.close();
    output.close();
    // change here
    encodeMapToBinaryFile(resultAsciiValue, "encoded_data");
    readBinaryFile("encoded_data.bin");
    combineFiles("encoded_data.txt", "HuffOutput.txt", "combined_file.txt");
    writeToBinaryFile("combined_file.txt");
}

void compress(string filename)
{
    std::ifstream inputFile(filename); // Open input file

    int windowSize = 10; // Adjust window size
    int bufferSize = 5;  // Adjust buffer size
    string lzoutput = "lzoutput.txt";
    // line to take input from input.txt
    std::string line;
    std::ofstream outputFile(lzoutput);
    if (inputFile.is_open())
    {
        while (std::getline(inputFile, line))
        {
            std::vector<LZ77Token> compressedData = compressLine(line, windowSize, bufferSize);
            for (const auto &token : compressedData)
            {
                outputFile << "(" << token.offset << "," << token.length << "," << token.nextChar << ")";
            }
        }
        inputFile.close(); // Close the file
    }
    else
    {
        std::cerr << "Error opening file." << std::endl;
    }
    outputFile.close();
    huffmanencode(lzoutput);
    fs::rename("combined_file.bin", changeExtension(filename, ".bin"));
    fs::remove("lzoutput.txt");
    fs::remove("combined_file.txt");
    fs::remove("combined_file.bin");
    fs::remove("encoded_data.bin");
    fs::remove("encoded_data.txt");
    fs::remove("HuffOutput.bin");
    fs::remove("HuffOutput.txt");
    fs::remove("output_file1.txt");
    fs::remove("output_file2.txt");
}
void decompressHuffman(string filename, const char *outputFileName, map<char, string> result)
{
    // Create a new map with reversed key-value pairs
    std::map<string, char> reverseValue;

    for (const auto &entry : result)
    {
        reverseValue[entry.second] = entry.first;
    }

    string currentCode;
    char c;
    ifstream heoutput(filename);
    ofstream hufmanoutput("HDoutput.txt");
    if (!hufmanoutput.is_open())
    {
        cout << "Error huffman output is not open.";
        return;
    }
    while (heoutput.get(c))
    {
        currentCode += c;

        // Check if the current code matches a Huffman code
        if (reverseValue.count(currentCode))
        {
            // Append the corresponding character to the decoded data
            hufmanoutput << reverseValue[currentCode];

            // Reset the current code
            currentCode.clear();
        }
    }
    heoutput.close();
    hufmanoutput.close();
    cout << outputFileName << endl;
    decompressLZ77(parseTuples("HDoutput.txt"), outputFileName);
}
void decompress(const char *filename)
{
    // make string here
    readBinaryFile(filename);
    separateFile(renameToText(filename), "output_file1.txt", "output_file2.txt");
    writeToBinaryFile("output_file1.txt");
    map<char, string> result = decodeBinaryFileToMap("output_file1.bin");

    std::cout << "\nDecoded Map:\n";
    for (const auto &entry : result)
    {
        std::cout << entry.first << ": " << entry.second << "\n";
    }
    decompressHuffman("output_file2.txt", renameToText(filename), result);
    fs::remove(filename);
    fs::remove("finaloutput.txt");
    fs::remove("HDoutput.txt");
    fs::remove("output_file1.txt");
    fs::remove("output_file2.txt");
    fs::remove("output_file1.bin");
}

// int main()
// {
//     compress("input.txt");
//     decompress("input.bin");
// }
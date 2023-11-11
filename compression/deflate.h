#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include "binarayconversion.h"
using namespace std;

struct LZ77Token
{
    int offset;    // Offset to the start of the match
    int length;    // Length of the match
    char nextChar; // Next character following the match
};

std::string decompressLZ77(const std::vector<LZ77Token>& tokens, const char* filename)
{
    std::string decompressedString;
    std::ofstream file(filename);
    for (const LZ77Token& token : tokens)
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
    file.close();
    return decompressedString;
}

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

// function for binary addition
string addBinary(string a, string b)
{
    std::string result = "";
    int carry = 0;

    // Make sure both strings have the same length by adding leading zeros if needed
    while (a.length() < b.length())
    {
        a = "0" + a;
    }
    while (b.length() < a.length())
    {
        b = "0" + b;
    }

    for (int i = a.length() - 1; i >= 0; i--)
    {
        int bit1 = a[i] - '0'; // Convert character to integer (0 or 1)
        int bit2 = b[i] - '0'; // Convert character to integer (0 or 1)

        int sum = bit1 + bit2 + carry;
        carry = sum / 2;
        result = to_string(sum % 2) + result;
    }

    if (carry > 0)
    {
        result = "1" + result;
    }

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
    // for (int i = 0; i < w.size(); i++)
    // {
    //     cout << i << " value " << w[i] << endl;
    // }
    w[1] = 0;
    for (int next = 2; next <= n - 1; ++next)
    {
        w[next] = w[w[next]] + 1;
    }
    // for (int i = 0; i < w.size(); i++)
    // {
    //     cout << i << " value " << w[i] << endl;
    // }
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
    // for (int i = 0; i < w.size(); i++)
    // {
    //     cout << i << " value " << w[i] << endl;
    // }
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
    // for (int i = 0; i < prefixCode.size(); i++)
    // {
    //     cout << i << "i th code " << prefixCode[i] << endl;
    // }
    for (int i = 0; i < z.size(); i++)
    {
        asciiValue[z[i]] = prefixCode[i];
    }
    // Iterate over the elements using iterators
    // for (auto it = asciiValue.begin(); it != asciiValue.end(); ++it)
    // {
    //     std::cout << it->first << " => " << it->second << std::endl;
    // }
    return asciiValue;
}

// convert string of tupples to token
vector<LZ77Token> stringToToken(const char* filename)
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
        // for (const auto &token : tokens)
        // {
        //     std::cout << "Offset: " << token.offset << " Length: " << token.length << " Next Char: " << token.nextChar << std::endl;
        // }
    }
    else
    {
        std::cerr << "Error opening file." << std::endl;
    }
    return tokens;
}

std::vector<LZ77Token> parseTuples(const std::string& filename) {
    std::vector<LZ77Token> tokens;

    // Open the file for reading
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open the file.\n";
        return tokens;
    }

    // Read each tuple from the file
    char c;
    std::ofstream ofs("finaloutput.txt");
    while (inputFile.get(c)) {
        if (c == '(') {
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
        } else {
            ofs << c;  // Include non-tuple characters
        }
    }

    // Close the file
    inputFile.close();
    ofs.close();

    return tokens;
}

/*
int main()
{
    std::ifstream inputFile("input.txt"); // Open input file
    std::string line;

    int windowSize = 10; // Adjust window size
    int bufferSize = 5;  // Adjust buffer size
    std::ofstream outputFile("lzoutput.txt");
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
    outputFile.close(); // Close the file
    // huffman encoding starting
    unordered_map<char, long> charOccurence;
    ifstream inputFil("lzoutput.txt");
    if (!inputFil.is_open())
    {
        cout << "error opening file " << endl;
        return 1;
    }

    char ch;

    while (inputFil.get(ch))
    {
        charOccurence[ch]++;
    }
    // charOcc.sort()

    inputFil.close();
    multimap<long, char> sortedMap;
    for (const auto &pair : charOccurence)
    {
        sortedMap.insert(make_pair(pair.second, pair.first));
    }

    // Print the sorted multimap
    // Traverse the multimap in reverse order
    // vector to store character frequency
    vector<long> charFrequencyInDecreasingOrder;
    //
    vector<char> charInDecreasingOrderOfFrequency;
    for (auto rit = sortedMap.rbegin(); rit != sortedMap.rend(); ++rit)
    {
        charFrequencyInDecreasingOrder.push_back(rit->first);
        charInDecreasingOrderOfFrequency.push_back(rit->second);
    }
    for (int i = 0; i < charFrequencyInDecreasingOrder.size(); i++)
    {
        cout << charFrequencyInDecreasingOrder[i] << endl;
    }
    for (int i = 0; i < charFrequencyInDecreasingOrder.size(); i++)
    {
        cout << charInDecreasingOrderOfFrequency[i] << endl;
    }

    ifstream decodeFile("lzoutput.txt");

    if (!decodeFile.is_open())
    {
        cout << "error opening file " << endl;
        return 1;
    }
    char c;
    map<char, string> resultAsciiValue = calcHuffLens(charFrequencyInDecreasingOrder, charInDecreasingOrderOfFrequency);
    // Create a new map with reversed key-value pairs
    std::map<string, char> reverseValue;

    for (const auto &entry : resultAsciiValue)
    {
        reverseValue[entry.second] = entry.first;
    }

    std::ofstream output("HCoutput.txt");
    if (output.is_open())
    {
        while (decodeFile.get(c))
        {
            output << resultAsciiValue[c];
        }
        decodeFile.close();
        output.close();
        writeToBinaryFile("HCoutput.txt");
        std::cout << "Data written to "
                  << "HCoutput.txt" << std::endl;
    }
    else
    {
        std::cerr << "Error opening the file for writing." << std::endl;
    }

    // // huffman encoding end

    // huffman decoding start
    ofstream hufmanoutput("HDoutput.txt");
    if (!hufmanoutput.is_open())
    {
        cout << "Error huffman output is not open.";
        return 0;
    }
    string currentCode;
    readBinaryFile("output.bin", "HCoutput.txt");
    ifstream heoutput("HCoutput.txt");
    while(heoutput.get(c))
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
    hufmanoutput.close();
    heoutput.close();
    // huffman decoding end
    decompressLZ77(parseTuples("HDoutput.txt"), "finaloutput.txt");
    return 0;
}
*/
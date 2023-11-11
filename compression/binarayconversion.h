#include <iostream>
#include <fstream>


bool writeToBinaryFile(const char* filename){
    // Your string containing '1' and '0'
    std::string binaryString = "110101010";
    std::ifstream inputFile(filename);
    if(!inputFile.is_open()){
        std::cout <<" Fail to generate binary file due to input file not open" << std::endl;
        return false;
    }
    // Open the binary file for writing
    std::ofstream binaryFile("output.bin", std::ios::binary);

    // Check if the file is open
    if (!binaryFile.is_open()) {
        std::cerr << "Unable to open the file.\n";
        return false;
    }

    // Variable to store packed bits
    char packedBits = 0, c;
    // Variable to keep track of the bit position in the packed byte
    int bitPosition = 0;

    // Iterate through the characters in the string
    while (inputFile.get(c)) {
        // Set the corresponding bit in the packed byte
        packedBits |= (c == '1') ? (1 << bitPosition) : 0;

        // Move to the next bit position
        bitPosition++;

        // If we've filled a byte, write it to the file
        if (bitPosition == 8) {
            binaryFile.write(&packedBits, sizeof(char));

            // Reset the packedBits and bitPosition for the next byte
            packedBits = 0;
            bitPosition = 0;
        }
    }

    // If there are remaining bits, write them to the file
    if (bitPosition > 0) {
        binaryFile.write(&packedBits, sizeof(char));
    }

    // Close the binary file
    binaryFile.close();
    return true;
}

bool readBinaryFile(const char* filename, const char* outfilename) {
    // Open the binary file for reading
    std::ifstream binaryFile(filename, std::ios::binary);
    if (!binaryFile.is_open()) {
        std::cerr << "Unable to open the file.\n";
        return false;
    }
    std::ofstream outfile(outfilename);

    // Variable to store unpacked bits
    char unpackedBits;
    // Variable to keep track of the bit position in the unpacked byte
    int bitPosition = 0;

    // Read the file byte by byte
    while (binaryFile.read(&unpackedBits, sizeof(char))) {
        // Iterate through the bits in the byte
        for (int i = 0; i < 8; i++) {
            // Extract the i-th bit from the byte
            char bit = (unpackedBits & (1 << i)) ? '1' : '0';
            outfile << bit;
            // Output the bit (you can modify this to store it in a string, for example)
            // std::cout << bit;

            // Move to the next bit position
            bitPosition++;

            // Output a space for better readability
            // if (bitPosition % 8 == 0)
            //     std::cout << ' ';
        }
    }

    // Close the binary file
    binaryFile.close();
    outfile.close();
    return true;
}

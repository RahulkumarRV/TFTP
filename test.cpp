#include <iostream>
#include<filesystem>
#include <fstream>
#include <string>
#include <string.h>
#include <unordered_set>
using namespace std;
namespace fs = std::filesystem;
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

void createDirectoryStructure(string path, const unordered_set<string> ignorePaths){
    ofstream directory_structure("directory_structure.txt");
    listAllDirectories(0, path, ignorePaths, directory_structure);
    directory_structure.close();
}

std::string trimNewline(const std::string& str, char toTrim) {
    // Trim newline character from the end of the string
    if (!str.empty() && str[str.length()] == toTrim) {
        return str.substr(0, str.length() - 1);
    }
    return str;
}

// if the return emtpy list means may be the file .ignore file does not exist
// else return the list of directories and the files to ignore
void readNamesOfIgnoreFiles(std::unordered_set<std::string>& names) {
    
    std::ifstream file("./.ignore");
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

void generateDirectory(string path){
    unordered_set<string> paths;
    
    readNamesOfIgnoreFiles(paths);
    for(string it : paths) {
        cout << "-" << it << endl << strlen(it.c_str()) << endl;
    }
    createDirectoryStructure(path, paths);
}


int main(){

    fs::create_directory("/rahul");

    return 0;
}
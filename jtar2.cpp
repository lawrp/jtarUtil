
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include "file.h"

using namespace std;

// Function prototypes
void showHelp();
void createTarFile(const string& tarFileName, vector<string>& fileNames);
void listTarFile(const string& tarFileName);
void extractTarFile(const string& tarFileName);
void processDirectory(const string& dirPath, vector<File>& files);
void getFileInfo(const string& filePath, File& fileObj);
void writeFileToTar(ofstream& tarFile, const File& fileObj);
void writeDirectoryToTar(ofstream& tarFile, const string& dirPath, const string& basePath);
void recreateFile(ifstream& tarFile, const File& fileObj);
string formatTimestamp(time_t time);
string getFileSize(const string& filePath);
string getFileMode(const string& filePath);
string getFileTimestamp(const string& filePath);
void createDirectories(const string& path);
void isADir(const string& filePath);

int main(int argc, char* argv[]) {
    // Check if arguments are provided
    if (argc < 2) {
        cerr << "Error: Too few arguments." << endl;
        showHelp();
        return 1;
    }

    // Check for help option
    if (strcmp(argv[1], "--help") == 0) {
        showHelp();
        return 0;
    }

    // Check for valid option flag
    if (argc < 3 || argv[1][0] != '-') {
        cerr << "Error: Invalid option." << endl;
        showHelp();
        return 1;
    }

    string option = argv[1];
    string tarFileName = argv[2];

    // Process based on option
    if (option == "-cf") {
        if (argc < 4) {
            cerr << "Error: No files specified for creating tar file." << endl;
            return 1;
        }
        
        vector<string> fileNames;
        for (int i = 3; i < argc; i++) {
            fileNames.push_back(argv[i]);
        }
        
        createTarFile(tarFileName, fileNames);
    } 
    else if (option == "-tf") {
        listTarFile(tarFileName);
    } 
    else if (option == "-xf") {
        extractTarFile(tarFileName);
    } 
    else {
        cerr << "Error: Invalid option." << endl;
        showHelp();
        return 1;
    }

    return 0;
}

// Display help information
void showHelp() {
    cout << "jtar - Java Tar Utility" << endl;
    cout << "Usage:" << endl;
    cout << "  jtar -cf tarfile file1 dir1...  Create a new tar file with specified files/directories" << endl;
    cout << "  jtar -tf tarfile               List the contents of a tar file" << endl;
    cout << "  jtar -xf tarfile               Extract files from a tar file" << endl;
    cout << "  jtar --help                    Display this help information" << endl;
}

// Create a tar file from a list of files and directories
void createTarFile(const string& tarFileName, vector<string>& fileNames) {
    ofstream tarFile(tarFileName, ios::binary);
    
    if (!tarFile.is_open()) {
        cerr << "Error: Could not create tar file " << tarFileName << endl;
        return;
    }
    
    // Process each file or directory
    for (const string& fileName : fileNames) {
        struct stat fileStat;
        
        if (stat(fileName.c_str(), &fileStat) != 0) {
            cerr << "Error: Cannot access " << fileName << endl;
            continue;
        }

        // Check if it's a directory
        if (S_ISDIR(fileStat.st_mode)) {
            // Process directory
            writeDirectoryToTar(tarFile, fileName, "");
        } else {
            // Process file
            File fileObj;
            getFileInfo(fileName, fileObj);
            writeFileToTar(tarFile, fileObj);
        }
    }
    
    tarFile.close();
    cout << "Successfully created " << tarFileName << endl;
}

// Get file information and populate File object
void getFileInfo(const string& filePath, File& fileObj) {
    struct stat fileStat;
    
    if (stat(filePath.c_str(), &fileStat) != 0) {
        cerr << "Error: Cannot access " << filePath << endl;
        return;
    }
    
    // Set file name (use basename)
    fileObj = File(
        filePath.c_str(), 
        getFileMode(filePath).c_str(), 
        getFileSize(filePath).c_str(), 
        getFileTimestamp(filePath).c_str()
    );
    
    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        fileObj.flagAsDir();
    }
}

// Write file information and content to tar file
void writeFileToTar(ofstream& tarFile, const File& fileObj) {
    // Write file information
    tarFile.write(fileObj.getName().c_str(), fileObj.getName().length() + 1);  // Include null terminator
    tarFile.write(fileObj.getPmode().c_str(), fileObj.getPmode().length() + 1);
    tarFile.write(fileObj.getSize().c_str(), fileObj.getSize().length() + 1);
    tarFile.write(fileObj.getStamp().c_str(), fileObj.getStamp().length() + 1);
    
    // Write directory flag
    int dirFlag = fileObj.isADir() ? 1 : 0;
    tarFile.write(reinterpret_cast<const char*>(&dirFlag), sizeof(int));
    
    // If not a directory, write file content
    if (!fileObj.isADir()) {
        ifstream inputFile(fileObj.getName(), ios::binary);
        if (inputFile.is_open()) {
            int fileSize = stoi(fileObj.getSize());
            char* buffer = new char[fileSize];
            
            inputFile.read(buffer, fileSize);
            tarFile.write(buffer, fileSize);
            
            delete[] buffer;
            inputFile.close();
        } else {
            cerr << "Error: Could not read file " << fileObj.getName() << endl;
        }
    }
}

// Process directory recursively and write to tar file
void writeDirectoryToTar(ofstream& tarFile, const string& dirPath, const string& basePath) {
    DIR* dir = opendir(dirPath.c_str());
    
    if (!dir) {
        cerr << "Error: Could not open directory " << dirPath << endl;
        return;
    }
    
    // Create File object for the directory itself
    File dirObj;
    getFileInfo(dirPath, dirObj);
    
    // Use relative path if basePath is provided
    string relativePath = basePath.empty() ? dirPath : basePath + "/" + dirPath;
    
    // Write directory entry to tarFile
    writeFileToTar(tarFile, dirObj);
    
    struct dirent* entry;
    
    // Process all entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        
        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }
        
        string fullPath = dirPath + "/" + name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) != 0) {
            cerr << "Error: Cannot access " << fullPath << endl;
            continue;
        }
        
        // Recursively process subdirectories
        if (S_ISDIR(fileStat.st_mode)) {
            writeDirectoryToTar(tarFile, fullPath, relativePath);
        } else {
            // Process file
            File fileObj;
            getFileInfo(fullPath, fileObj);
            writeFileToTar(tarFile, fileObj);
        }
    }
    
    closedir(dir);
}

// List contents of a tar file
void listTarFile(const string& tarFileName) {
    ifstream tarFile(tarFileName, ios::binary);
    
    if (!tarFile.is_open()) {
        cerr << "Error: Could not open tar file " << tarFileName << endl;
        return;
    }
    
    while (tarFile.peek() != EOF) {
        char name[81];
        char pmode[5];
        char size[7];
        char stamp[16];
        int dirFlag;
        
        // Read file information
        tarFile.getline(name, 81, '\0');
        tarFile.getline(pmode, 5, '\0');
        tarFile.getline(size, 7, '\0');
        tarFile.getline(stamp, 16, '\0');
        tarFile.read(reinterpret_cast<char*>(&dirFlag), sizeof(int));
        
        if (tarFile.eof() || tarFile.fail()) {
            break;
        }
        
        // Create file object
        File fileObj(name, pmode, size, stamp);
        if (dirFlag == 1) {
            fileObj.flagAsDir();
        }
        
        // Print file information
        cout << fileObj.printPmode() << " ";
        if (dirFlag == 1) {
            cout << "d ";
        } else {
            cout << "- ";
        }
        cout << fileObj.getSize() << " ";
        cout << fileObj.getStamp() << " ";
        cout << fileObj.getName() << endl;
        
        // Skip file content if not directory
        if (dirFlag == 0) {
            int fileSize = stoi(size);
            tarFile.seekg(fileSize, ios::cur);
        }
    }
    
    tarFile.close();
}

// Extract files from a tar file
void extractTarFile(const string& tarFileName) {
    ifstream tarFile(tarFileName, ios::binary);
    
    if (!tarFile.is_open()) {
        cerr << "Error: Could not open tar file " << tarFileName << endl;
        return;
    }
    
    while (tarFile.peek() != EOF) {
        char name[81];
        char pmode[5];
        char size[7];
        char stamp[16];
        int dirFlag;
        
        // Read file information
        tarFile.getline(name, 81, '\0');
        tarFile.getline(pmode, 5, '\0');
        tarFile.getline(size, 7, '\0');
        tarFile.getline(stamp, 16, '\0');
        tarFile.read(reinterpret_cast<char*>(&dirFlag), sizeof(int));
        
        if (tarFile.eof() || tarFile.fail()) {
            break;
        }
        
        // Create file object
        File fileObj(name, pmode, size, stamp);
        if (dirFlag == 1) {
            fileObj.flagAsDir();
        }
        
        // Create directories for the file path if needed
        string filePath = fileObj.getName();
        size_t lastSlash = filePath.find_last_of('/');
        if (lastSlash != string::npos) {
            createDirectories(filePath.substr(0, lastSlash));
        }
        
        // Create directory or file
        if (fileObj.isADir()) {
            // Create directory
            mkdir(fileObj.getName().c_str(), stoi(fileObj.getPmode(), nullptr, 8));
        } else {
            // Create file
            recreateFile(tarFile, fileObj);
        }
    }
    
    tarFile.close();
    cout << "Successfully extracted files from " << tarFileName << endl;
}

// Recreate a file from tar file
void recreateFile(ifstream& tarFile, const File& fileObj) {
    ofstream outputFile(fileObj.getName(), ios::binary);
    
    if (!outputFile.is_open()) {
        cerr << "Error: Could not create file " << fileObj.getName() << endl;
        return;
    }
    
    int fileSize = stoi(fileObj.getSize());
    char* buffer = new char[fileSize];
    
    tarFile.read(buffer, fileSize);
    outputFile.write(buffer, fileSize);
    
    delete[] buffer;
    outputFile.close();
    
    // Set file permissions
    chmod(fileObj.getName().c_str(), stoi(fileObj.getPmode(), nullptr, 8));
    
    // Set file timestamp using touch command
    string touchCmd = "touch -t " + fileObj.getStamp().substr(0, 12) + " " + fileObj.getName();
    system(touchCmd.c_str());
}

// Create all directories in a path
void createDirectories(const string& path) {
    string currentPath;
    size_t pos = 0;
    
    while ((pos = path.find('/', pos)) != string::npos) {
        currentPath = path.substr(0, pos);
        if (!currentPath.empty()) {
            mkdir(currentPath.c_str(), 0777);
        }
        pos++;
    }
    
    if (!path.empty()) {
        mkdir(path.c_str(), 0777);
    }
}

// Get file size as a string
string getFileSize(const string& filePath) {
    struct stat fileStat;
    
    if (stat(filePath.c_str(), &fileStat) != 0) {
        return "0";
    }
    
    return to_string(fileStat.st_size);
}

// Get file mode as a string
string getFileMode(const string& filePath) {
    struct stat fileStat;
    
    if (stat(filePath.c_str(), &fileStat) != 0) {
        return "0644";
    }
    
    char modeStr[5];
    sprintf(modeStr, "%04o", fileStat.st_mode & 0777);
    return string(modeStr);
}

// Get file timestamp as a string in the format YrMonthDayHrMin.Sec
string getFileTimestamp(const string& filePath) {
    struct stat fileStat;
    
    if (stat(filePath.c_str(), &fileStat) != 0) {
        return "197001010000.00";
    }
    
    struct tm* timeInfo = localtime(&fileStat.st_mtime);
    char timestamp[16];
    
    sprintf(timestamp, "%04d%02d%02d%02d%02d.%02d",
            timeInfo->tm_year + 1900,
            timeInfo->tm_mon + 1,
            timeInfo->tm_mday,
            timeInfo->tm_hour,
            timeInfo->tm_min,
            timeInfo->tm_sec);
    
    return string(timestamp);
}
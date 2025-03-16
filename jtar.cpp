#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include "file.h"

using namespace std;

// Function prototypes
void showHelp();
bool isADirectory(const string& path);
void createTar(const string& tarFileName, const vector<string>& fileNames);
File createFileObj(const string& fileName);
char* getFileMode(const string& fileName);
char* getFileSize(const string& fileName);
char* getFileTimestamp(const string& fileName);

int main(int argc, char *argv[]) {
    string flag = argv[1];
    bool xflag = false, cflag = false, tflag = false;

    // First check if we have enough arguments
    if (argc < 2) {
        cerr << "Error: Too few arguments." << endl;
        showHelp();
        return 1;
    }

    // Parse the flag
    if (strcmp(argv[1], "--help") == 0) {
        showHelp();
        return 0;
    } else if (flag[0] == '-') {
        if (flag.substr(1) == "xf") {
            xflag = true;
            cout << "Reading a tar file and recreating all the files saved in the tar file." << endl;
        } else if (flag.substr(1) == "cf") {
            cflag = true;
            cout << "Creating a tar file named tarfile." << endl;
        } else if (flag.substr(1) == "tf") {
            tflag = true;
            cout << "Listing all the filenames in the tar file." << endl;
        } else {
            cout << "Invalid flag" << endl;
            showHelp();
            return 1;
        }
    } else {
        cout << "Invalid flag" << endl;
        showHelp();
        return 1;
    }

    // Check if we have enough arguments based on the flag
    if ((xflag || tflag) && argc < 3) {
        cerr << "Error: Tar filename not specified." << endl;
        return 1;
    }

    if (cflag && argc < 4) {
        cerr << "Error: No files specified for creating tar file." << endl;
        return 1;
    }

    // Get tarfile name
    string tarFileName = argv[2];

    // If creating a tar file, process input files
    vector<string> fileNames;
    if (cflag) {
        for (int i = 3; i < argc; i++) {
            fileNames.push_back(argv[i]);
        }
        createTar(tarFileName, fileNames);
    }

    return 0;
}

// Display help information
void showHelp() {
    cout << "jtar - jTar Utility" << endl;
    cout << "Usage:" << endl;
    cout << "  jtar -cf tarfile file1 dir1...  Create a new tar file with specified files/directories" << endl;
    cout << "  jtar -tf tarfile               List the contents of a tar file" << endl;
    cout << "  jtar -xf tarfile               Extract files from a tar file" << endl;
    cout << "  jtar --help                    Display this help information" << endl;
}

bool isDirectory(const string& path) {
    string command = "stat " + path + " | grep 'directory' &> /dev/null";
    return system(command.c_str()) == 0;
}

void createTar(const string& tarFileName, const vector<string>& fileNames) {
    ofstream tarFile(tarFileName, ios::binary);

    if (!tarFile.is_open()) {
        cerr << "Error: Could not create tar file " << tarFileName << endl;
        return;
    }

    // Process each file or directory
    for (const string& fileName : fileNames) {
        
        File fileObj;
        
        // Check if it's a directory
        if (isDirectory(fileName)) {
            // Process directory
            writeDirectoryToTar(tarFile, fileName, "");
        } else {
            createFileObj(fileName);
            writeFileToTar(tarFile, fileObj);
        }
    }

    tarFile.close();
    cout << "Successfully created " << tarFileName << endl;
}

File createFileObj(const string& fileName) {
    struct stat fileStat;
    File fileObj;
    
    if (stat(fileName.c_str(), &fileStat) != 0) {
        cerr << "Error: Cannot access " << fileName << endl;
        return fileObj;
    }
    
    // Set file name (use basename)
    fileObj = File(
        fileName.c_str(), 
        getFileMode(fileName), 
        getFileSize(fileName), 
        getFileTimestamp(fileName)
    );
    
    // Check if it's a directory
    if (isDirectory(fileName)) {
        fileObj.flagAsDir();
    }
    
    return fileObj;
}

char* getFileMode(const string& fileName) {
    struct stat fileStat;
    
    if (stat(fileName.c_str(), &fileStat) != 0) {
        cerr << "Error: Cannot access " << fileName << endl;
        return "";
    }
    
    char* mode = new char[5];
    snprintf(mode, 5, "%04o", fileStat.st_mode & 0777);
    
    return mode;
}

char* getFileSize(const string& fileName) {
    struct stat fileStat;
    
    if (stat(fileName.c_str(), &fileStat) != 0) {
        cerr << "Error: Cannot access " << fileName << endl;
        return "";
    }
    
    char* size = new char[7];
    snprintf(size, 7, "%06ld", fileStat.st_size);
    
    return size;
}
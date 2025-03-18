#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <filesystem>
#include <utime.h>
#include "file.h"
#include <map>

using namespace std;

// Function prototypes
void showHelp();
bool isADirectory(const string& path);
void createTar(const string& tarFileName, const vector<File>& fileObjs);
File createFileObj(const string& fileName);
File createFileObj(const char* filename);
vector<string> listAll(vector<string> filenames);
vector<string> getDirectoryContents(const string& dirPath);
vector<File> getFileObjs(vector<string>& inputFiles);
void readTar(const string& tarFileName);
unsigned int getFileSize(ifstream& file);
void extractTar(const string& tarFileName);

int main(int argc, char *argv[]) {
    bool xflag = false, cflag = false, tflag = false;

    // First check if we have enough arguments
    if (argc < 2) {
        cerr << "Error: Too few arguments." << endl;
        showHelp();
        return 1;
    }
    string flag = argv[1];
    // Then Parse the flag
    if (flag == "--help") {
        showHelp();
        return 0;
    } else if (flag[0] == '-') {
        if (flag.substr(1) == "xf") {
            xflag = true;
        } else if (flag.substr(1) == "cf") {
            cflag = true;
        } else if (flag.substr(1) == "tf") {
            tflag = true;
        } else {
            cout << "Invalid flag: " << flag << ". Try running with --help for instructions." << endl << endl;
            return 1;
        }
    } else {
        cout << "No flag supplied on the command line. Try running --help for instructions." << endl;
        showHelp();
        return 1;
    }

    // Check if we have enough arguments based on the flag
    if ((xflag || tflag) && argc < 3) {
        cerr << "Error: Tar filename not specified." << endl;
        return 1;
    }

    // cflag must have at least 2 files to tar together.
    if (cflag && argc < 4) {
        cerr << "Error: No files specified for creating tar file." << endl;
        return 1;
    }

    // Get tarfile name, will always be 2nd argument.
    string tarFileName = argv[2];

    // If cflag, get the input files/directories, make file objects then create the tar file.
    vector<string> fileNames;
    if (cflag) {
        for (int i = 3; i < argc; i++) {
            fileNames.push_back(argv[i]);
        }
        vector<File> FileObjs = getFileObjs(fileNames);
        createTar(tarFileName, FileObjs);
    }

    // If tflag, list all the files in the tar file.
    if (tflag) {
        readTar(tarFileName);
    }

    // If xflag, extract all the files from the tar file to the current directory.
    if (xflag) {
        extractTar(tarFileName);
    }

    return 0;
}

// Display help information
void showHelp() {
    cout << "'jtar' saves many files together into a single tape or disk archive, and can restore individual files from the archive." << endl << endl;
    cout << "Usage: tar [OPTION]... [FILE]..." << endl << endl;
    cout << "  jtar -cf tarfile file1 dir1... Create a new tar file with specified files/directories" << endl;
    cout << "  jtar -tf tarfile               List the contents of a tar file" << endl;
    cout << "  jtar -xf tarfile               Extract files from a tar file" << endl;
    cout << "  jtar --help                    Display this help information" << endl << endl;
}

// Check if a path is a directory
bool isDirectory(const string& path) {
    struct stat buf;
    if (stat(path.c_str(), &buf) != 0) return false;
    return S_ISDIR(buf.st_mode);
}

// Create a tar file from a list of file objects
void createTar(const string& tarFileName, const vector<File>& files) {
    ofstream tarFile(tarFileName, ios::binary);
    if (!tarFile) {
        cerr << "Error: Unable to create tar file!" << endl;
        return;
    }

    for (const File& fileObj : files) {
        // Write the file object (this holds the metadata)
        tarFile.write(reinterpret_cast<const char*>(&fileObj), sizeof(File));

        // Write separator character so we know that we're in the content section
        tarFile.put('|');

        // Open and write the actual file contents
        ifstream inFile(fileObj.getName(), ios::binary);
        if (!inFile) {
            cerr << "Warning: Cannot open " << fileObj.getName() << " for reading." << endl;
            continue;
        }

        // Copy the file contents one char at a time
        char buffer;
        while (inFile.get(buffer)) {
            tarFile.put(buffer);
        }

        // Using '~' as a terminator for the content
        tarFile.put('~');

        inFile.close();
    }

    tarFile.close();
}

// Read a tar file and list all the files in it from the tarFile name
void readTar(const string& tarFileName) {
    ifstream tarFile(tarFileName, ios::binary);
    vector<string> filenames;
    unsigned int fileSize = getFileSize(tarFile);
    if (!tarFile) {
        cerr << "cannot open your tar file: " << tarFileName << " for reading. Are you sure this is the right name of your TarFile?" << endl;
    }
    unsigned int currentPos = 0;

    // Read the file objects until we reach the end of the file
    while (currentPos < fileSize) {
        // Read the File object one at a time
        File fileObj;
        tarFile.read(reinterpret_cast<char*>(&fileObj), sizeof(File));
        
        // Update position and check if we read successfully
        currentPos += sizeof(File);
        if (tarFile.gcount() != sizeof(File) || currentPos > fileSize) {
            cerr << "Warning: Incomplete File object read" << endl;
            break;
        }

        // Read the separator
        char separator;
        tarFile.get(separator);
        currentPos += 1;
        if (separator != '|' || currentPos > fileSize) {
            cerr << "Warning: Invalid tar format - expected '|' separator" << endl;
            break;
        }

        // Add the filename to our list
        filenames.push_back(fileObj.getName());

        // Skip the variable-length content until we reach the '~' marker
        char buffer;
        bool foundTerminator = false;
        while (currentPos < fileSize) {
            tarFile.get(buffer);
            currentPos += 1;
            if (buffer == '~') {
                foundTerminator = true;
                break;
            }
        }

        // Check if we ran out of file before finding the terminator
        if (!foundTerminator) {
            cerr << "Warning: Invalid tar format - missing '~' terminator" << endl;
            break;
        }
    }

    tarFile.close();

    // Print the filenames
    if (filenames.empty()) {
        cout << "No files found in the tar archive." << endl;
    } else {
        for (const string& name : filenames) {
            cout << name << endl;
        }
    }
}
 // Extract all the files from a tar file to the current directory
void extractTar(const string& tarFileName) {
    ifstream tarFile(tarFileName, ios::binary);
    if (!tarFile) {
        cerr << "Error: Cannot open " << tarFileName << ". Are you sure this is the right name of your TarFile?" << endl;
        return;
    }
    // Get the size of the file
    unsigned int fileSize = getFileSize(tarFile);
    map<string, string> dirTimestamps;
    
    // Read the file objects until we reach the end of the file
    while (tarFile.tellg() < fileSize) {
        // Read the File object
        File fileObj;
        tarFile.read(reinterpret_cast<char*>(&fileObj), sizeof(File));
        
        // Read the separator
        char separator;
        tarFile.get(separator);
        if (separator != '|') {
            cerr << "Error: Invalid format, expected '|'" << endl;
            break;
        }
        
        // Get the metadata found in the File Object
        string path = fileObj.getName();
        string permissions = fileObj.getPmode();
        string timestamp = fileObj.getStamp();
        
        // Handle directories
        if (fileObj.isADir()) {
            // Create directory (recursively if needed)
            string mkdirCmd = "mkdir -p '" + path + "'";
            system(mkdirCmd.c_str());

            // add timestamp to map
            dirTimestamps[path] = timestamp;
            
            // Skip to terminator character
            char c;
            while (tarFile.get(c) && c != '~');
        } 
        // Handle files
        else {
            // Extract file content and write it to the current file. 
            ofstream outFile(path, ios::binary);
            char c;
            while (tarFile.get(c) && c != '~') {
                outFile.put(c);
            }
            outFile.close();
        }
        
        // set permissions with a chmod command
        string chmodCmd = "chmod " + permissions + " '" + path + "'";
        system(chmodCmd.c_str());
        
        // Then set timestamp with touch -t command. 
        string touchCmd = "touch -t " + timestamp + " '" + path + "'";
        system(touchCmd.c_str());

    }
    tarFile.close();

    // Set the timestamps for the directories again after all the files have been extracted
    for (const auto& [dirPath, timestamp] : dirTimestamps) {
        string touchCmd = "touch -t " + timestamp + " '" + dirPath + "'";
        system(touchCmd.c_str());
    }
}


// a lot of the code in this function was taken from the utility.cpp file provided by Dr. Digh
File createFileObj(const char* filename) {
    struct stat buf;
    int result = stat(filename, &buf);
    
    // Check if file exists
    if (result != 0) {
        cerr << "Error: Cannot access file " << filename << ". Are you sure this is the right filename?" << endl;
        return File(); // Return empty File object if it doesn't
    }
    
    File fileObj;
    
    // Set name field
    if (strlen(filename) < 81) {
        fileObj = File(filename, "", "", "");
    } else {
        cerr << "Error: Filename too long" << endl;
        return File(); // Return empty File object if name is too long
    }
    
    // Handle regular files
    if (S_ISREG(buf.st_mode)) {
        // start with permission mode field
        char pmode[5];
        sprintf(pmode, "%1d%1d%1d", 
                (buf.st_mode & S_IRWXU) >> 6, 
                (buf.st_mode & S_IRWXG) >> 3, 
                (buf.st_mode & S_IRWXO));
        
        // next we get size field
        char size[7];
        snprintf(size, sizeof(size), "%06d", static_cast<int>(buf.st_size));
        
        // finally we get timestamp formatted as specified in the utility.cpp file provided.
        char stamp[16];
        strftime(stamp, 16, "%Y%m%d%H%M.%S", localtime(&buf.st_ctime));
        
        // Create File object with all the information
        fileObj = File(filename, pmode, size, stamp);
    }
    // Handle directories
    else if (S_ISDIR(buf.st_mode)) {
        char pmode[5];
        sprintf(pmode, "%1d%1d%1d", 
                (buf.st_mode & S_IRWXU) >> 6, 
                (buf.st_mode & S_IRWXG) >> 3, 
                (buf.st_mode & S_IRWXO));
        
        // For directories, we set size to 0
        char size[7] = "000000";
        
        char stamp[16];
        strftime(stamp, 16, "%Y%m%d%H%M.%S", localtime(&buf.st_ctime));
        
        // Create the directory and flag it as such
        fileObj = File(filename, pmode, size, stamp);
        fileObj.flagAsDir();
    }
    
    return fileObj;
}
// overloading function in case I pass it a string instead of a c-string.
File createFileObj(const string& filename) {
    return createFileObj(filename.c_str());
}
// recursively list all files in a directory by calling itself on subdirectories
vector<string> listAll(vector<string> files) {
    vector<string> allFiles;
    
    for (const string& file : files) {
        File fileObj = createFileObj(file);
        if (fileObj.getName().empty()) {  // Skip invalid entries
            continue;
        }
        
        allFiles.push_back(file);  // Add the file itself
        
        if (fileObj.isADir()) {
            // Get directory contents and process them recursively
            vector<string> dirContents = getDirectoryContents(file);
            vector<string> subFiles = listAll(dirContents);
            allFiles.insert(allFiles.end(), subFiles.begin(), subFiles.end());
        }
    }
    
    return allFiles;
}
// get all the files in a directory
// took this filesystem approach from GeeksforGeeks:
// https://www.geeksforgeeks.org/cpp-program-to-get-the-list-of-files-in-a-directory/?ref=ml_lbp
vector<string> getDirectoryContents(const string& dirPath) {
    vector<string> filepaths;

    try {
        for (const auto& entry : filesystem::directory_iterator(dirPath)) {
            filepaths.push_back(entry.path().string());
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error accessing directory: " << e.what() << endl;
    }

    return filepaths;
}

// get all the file objects from the input files (directories are handled recursively)
vector<File> getFileObjs(vector<string>& inputFiles) {
    vector<File> fileObjs;
    vector<string> allFileNames = listAll(inputFiles);
    for (const string& fileName : allFileNames) {
        File fileObj = createFileObj(fileName);
        fileObjs.push_back(fileObj);
    }
    return fileObjs;
}
// get the size of a file
unsigned int getFileSize(ifstream& file) {
    unsigned int size;
    file.seekg(0, ios::end);
    size = file.tellg();
    file.seekg(0, ios::beg);
    return size;
}

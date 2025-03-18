
#include "file.h"

using namespace std;

bool File::isADir() const
{
	return ADir;
}

void File::flagAsDir()
{
	ADir = true;
}

int File::recordSize() const
{
	return (strlen(name)+strlen(pmode)+strlen(size)+strlen(stamp));
}

string File::getName() const
{
	return name;
}

string File::getPmode() const
{
	return pmode;
}

string File::getStamp() const
{
	return stamp;
}

string File::getSize() const
{
	return size;
}

File::File ()
{
       strcpy (name, "\0");  strcpy (pmode, "\0");
       strcpy (size, "\0"); strcpy (stamp, "\0");
       ADir = false;

}

File::File (const char myName[], const char myPmode[],
            const char mySize[], const char myStamp[])
{
       strcpy (name, myName);  strcpy (pmode, myPmode);
       strcpy (size, mySize); strcpy (stamp, myStamp);
       ADir = false;
}

File & File::operator = (const File& otherFile)
{
       strcpy (name, otherFile.name);
       strcpy (pmode, otherFile.pmode);
       strcpy (size, otherFile.size);
       strcpy (stamp, otherFile.stamp);
       ADir = otherFile.ADir;
       return *this;
}
       
File::File (const File& otherFile)
{
       strcpy (name, otherFile.name);
       strcpy (pmode, otherFile.pmode);
       strcpy (size, otherFile.size);
       strcpy (stamp, otherFile.stamp);
       ADir = otherFile.ADir;
}

string File::printPmode() const {
    // First, determine the actual length of the pmode string
    int length = 0;
    while (length < 5 && pmode[length] != '\0') {
        length++;
    }
    
    // Basic validation - we need at least one digit
    if (length < 1) {
        return "Invalid mode";
    }
    
    string permissions = "-";  // Default: Assume it's a regular file
    
    const string permissionBits[8] = {
        "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"
    };
    
    // Process up to 3 digits
    int digits = min(length, 3);
    for (int i = 0; i < digits; i++) {
        // Check if this is a valid octal digit
        if (pmode[i] < '0' || pmode[i] > '7') {
            return "Invalid mode";
        }
        
        // Convert the digit to an index and append the corresponding permission string
        int index = pmode[i] - '0';
        permissions += permissionBits[index];
    }
    
    return permissions;
}

   void File::print() const {
       cout << "Name: " << name << endl;
       cout << "Permissions: " << pmode << ", " << printPmode() << endl;
       cout << "Size: " << size << endl;
       cout << "Timestamp: " << stamp << endl;
       cout << "Is a directory: " << (ADir ? "Yes" : "No") << endl;
   }


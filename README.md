# jtar - A Simple Tar Archiver

`jtar` is a lightweight linux utility that saves multiple files and directories into a single TarFile archive file and can extract files from existing archives.

## Features

- Create TarFile archives from files and directories
- List contents of TarFile archives
- Extract files and directories from TarFile archives
- Preserves file metadata (permissions, timestamps)

## Usage

### Options

- `jtar -cf tarfile file1 dir1...` Create a new tar file with specified files/directories
- `jtar -tf tarfile` List the contents of a tar file
- `jtar -xf tarfile` Extract files from a tar file
- `jtar --help` Display help information

## How It Works

### Creating TarFiles (`-cf`)

When creating a TarFile archive, `jtar`:

1. Recursively processes all input files and directories
2. For each file/directory, creates a File object containing metadata
3. Writes each File object to the TarFile archive followed by a separator character ('|')
4. Appends the actual file content
5. Finishes with a terminator character ('~')

### Listing TarFiles (`-tf`)

When listing a TarFile archive, `jtar`:

1. Reads each File object from the TarFile archive
2. Displays the name of each file/directory
3. Navigates past file content using separator and terminator characters

### Extracting TarFiles (`-xf`)

When extracting a TarFile archive, `jtar`:

1. Reads each File object from the TarFile archive
2. Creates directories or files as indicated by the File object metadata
3. Extracts file contents to their original paths
4. Applies original permissions and timestamps using the stored metadata (using chmod and touch -t system commands)
5. Ensures directory timestamps are set correctly after extraction is complete

## TarFile Format

The jtar TarFile format consists of:
1. File object metadata (fixed size)
2. Separator character ('|')
3. File content (variable size)
4. Terminator character ('~')
5. Repeated for each file/directory in the TarFile.

## File Class

### File Object Structure

At the core of `jtar` is the `File` class which represents metadata for each file or directory in the TarFile:

- `name`: The file or directory name (path) - This is a c-string with fixed length of 80 chars. 
- `pmode`: Permission mode stored in octal format (e.g., "755") - This is a c-string with fixed length of 4 chars.
- `size`: File size in bytes, formatted as a 6-digit string - This is a c-string with fixed length of 7 chars. (1MB max size)
- `stamp`: Timestamp in format YYYYMMDDHHMM.SS - This is a c-string with fixed length of 15 chars. 
- `ADir`: Boolean flag indicating whether the entry is a directory

Each File object is created and placed into the TarFile archive along with the corresponding file content.

### File Class Methods

The File class provides several utility methods:

- `isADir()`: Checks if the entry is a directory
- `flagAsDir()`: Marks the entry as a directory
- `getName()`, `getPmode()`, `getStamp()`, `getSize()`: Getters for metadata fields
- `printPmode()`: Converts numeric permissions to readable format (e.g., "rwxr-xr-x")
- `print()`: Outputs detailed information about the file object
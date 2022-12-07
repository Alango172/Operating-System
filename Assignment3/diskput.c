/*
    Name: Franco Cai
    vId: V00940471
    Subject: CSC360
    Assignment 3 part 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define BYTES_PER_SECTOR 512
#define POSSIBLE_DIRECTORY_ENTRIES 224 //total possible entries of root directory
#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

int firstLogicalCluster;

/*
    The helper functions of pringting the list of directories
*/

int getFileSize(char* p) {
    int fileSize = (p[28] & 0xff) + ((p[29] & 0xff) << 8) + ((p[30] & 0xff) << 16) + ((p[31] & 0xff) << 24);
    return fileSize;
}

// check the fat value on each directory entry in FAT table
int getFatValue(int n, char* p) {
    int fourBit, eightBit;
    // combine the two numbers by making space for the eight bit
    if((n % 2) == 0) {
        fourBit = p[1+(3*n)/2] & 0x0f;
        eightBit = p[(3*n)/2] & 0xff;
        return (fourBit << 8) + eightBit; // 11010011 + ****1111 -> 111111010011
    } else {
        fourBit = p[(3*n)/2] & 0xf0;
        eightBit = p[1+(3*n)/2] & 0xff;
        return (fourBit >> 4) + (eightBit << 4); // 11010011 + 1111**** -> 110100111111
    }
}

// update the entry in the FAT with the entry value and position passed
void updateFATValue(char* p, int position, int entryValue) {

    int fourBits, eightBits;

    if((position % 2) == 0) {
        fourBits = (entryValue >> 8) & 0x0F; // Low four bits
        eightBits = (entryValue & 0xFF) + (p[1 + ((3*position)/2)] & 0xF0); // 8 bits
        p[1 + (3*position)/2] = fourBits;
        p[(3*position)/2] = eightBits;
    } else {
        fourBits = ((entryValue << 4) & 0xF0) + (p[((3*position)/2)] & 0x0F); // High four bits
        eightBits = (entryValue >> 4) & 0xFF; // 8 bits
        p[(3*position)/2] = fourBits;
        p[1 + (3*position)/2] = eightBits;
    }
}

// check for the free sector in Fat table
int getFreeSector(char* p){
    int index = 2;
    while(getFatValue(index,p) != 0x0000){
        index++;
    }
    return index;
}

char* getFileName(char* p) {
    char* fileName = calloc(1,sizeof(char)*8);
    char* extention = calloc(1,sizeof(char)*3);

    for (int j = 0; j < 8; j++) {
        if (p[j] == ' ') {
            continue;
        }
        fileName[j] = p[j];
    }
    for(int j = 0; j < 3; j++) {
        extention[j] = p[j+8];
    }

    // combine the file name and the extention
    strcat(fileName,".");
    strcat(fileName, extention);
    
    return fileName;
}

// get the numebr to the total sector in the disk
int getTotalSector(char* p) {
    return (p[19] & 0x00FF) + ((p[20] & 0x00FF) << 8);
}

// function to remove spaces in the file name and extentions
void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

int getFreeSize(char* p) {
    int unusedEntries = 0;
    int size = getTotalSector(p);
    p += BYTES_PER_SECTOR;
    // Only count free sectors in the data sectors 
    for(int i = 2; i < size - 31; i++) {
        if (getFatValue(i,p) == 0x0000) {
            unusedEntries++;
        }
    }
    return unusedEntries * BYTES_PER_SECTOR;
}

void toUpperCase(char* name) {
    while (*name) {
    *name = toupper((unsigned char) *name);
    name++;
  }
}
// Get the logical cluster of this directory entry
int getLogicalCluster(char* p) {
    return (p[26] & 0x00FF) + ((p[27] & 0x00FF)<<8);
}

// A function for recursive call of subdirectory
int searchSubDir(char* p, char* fileName, int LC, char* path, char* dirName) {

    int basePos = (LC+ 31) * BYTES_PER_SECTOR;
    int firstBit = p[basePos];
    int counter = -1;
    int subDirRecord[POSSIBLE_DIRECTORY_ENTRIES];
    int recordCount = 0;
    char* subDirName = calloc(1,sizeof(char) *8);

    
    if(strcmp(dirName, path) == 0) { // if the path name equals to the current directory
        while(firstBit != 0x00 && counter < 16) {
            counter++;
            char* fileNameDisk = getFileName(p + basePos + counter*32);
           
            if(strcmp(fileNameDisk, fileName) == 0) {
                printf("Error: There is a file with same name in this directory\n");
                exit(1);
            } else {
                continue;
            }
        }

        return LC;
    }

    counter = 0;

    while(firstBit != 0x00 && counter < 16){

        counter++;
        firstBit = p[basePos + counter*32];
        int check = getLogicalCluster(p+ basePos + counter*32);
        if(firstBit == 0xE5 || (p[basePos + counter*32+11] & 0x08) == 0x08 || firstBit == '.' || check == 0 || check == 1) {
            continue;
        } 
        if(p[basePos+ counter*32 + 11] == 0x10) {
            subDirRecord[recordCount++] = counter;
        } else {
            continue;
        }
    }

    // A recursive call for all the subdirectories
    for(int i = 0; i < recordCount; i++) {
        char* previousPath = calloc(1, sizeof(char)*9+strlen(dirName));
        strcat(previousPath, dirName);  
        counter = subDirRecord[i];
        for(int j = 0; j < 8; j++) {
            if (p[basePos + counter* 32 +j] == ' ') {
                continue;
            }
            subDirName[j] = p[basePos + counter* 32 + j];
        }
        strcat(previousPath,"/");
        strcat(previousPath,subDirName);

        int logical_cluster = getLogicalCluster(p + basePos + counter*32);  
        return searchSubDir(p, fileName, logical_cluster, path, previousPath);
    }  
}

int searchFile(char* p, char* path, char* fileName) {
    
    int basePos = BYTES_PER_SECTOR * 19;
    int counter = -1;
    int firstBit = p[basePos];
    int subDirRecord[POSSIBLE_DIRECTORY_ENTRIES];
    int recordCount =0;
    char* subDirName = calloc(1,sizeof(char)*8);
    int result;

    while(firstBit != 0x00 && counter < POSSIBLE_DIRECTORY_ENTRIES){

        counter++;
        firstBit = p[basePos + counter*32+32];
        if(p[basePos+ counter*32 + 11] == 0x10) {
            subDirRecord[recordCount++] = counter;
            continue;
        } else {
            if(path[0] == '\0') {
                char* fileNameDisk = getFileName(p + basePos + counter*32);
                if(strcmp(fileNameDisk, fileName) == 0) {
                    printf("Error: There is a file with same name in the root directory\n");
                    exit(1);
                }
            }
        }
    }

    // if the the command did not insert a path, we do not need to check the subdirectories
    if(path[0] == '\0') {
        return 0;
    }

    // If there is a subdirecory and the file is not found, search into the subdir
    for(int i = 0; i < recordCount; i++) {
        counter = subDirRecord[i];
        for(int j = 0; j < 8; j++) {
            if (p[basePos + counter* 32 +j] == ' ') {
                continue;
            }
            subDirName[j] = p[basePos + counter* 32 + j];
        }
        // allocate memory for the subdirecory name that has exactly 9 char to avoid messy code when passing arguments
        char* subDir = calloc(1, sizeof(char)* 9);
        strcat(subDir,"/");
        strcat(subDir,subDirName);
        int logical_cluster = getLogicalCluster(p + basePos + counter*32);
        result = searchSubDir(p, fileName, logical_cluster,path, subDir);
        if(result > 0){
            return result;
        } else {
            return -1;
        }
    }
}

// Copy the content of the source file to the dest file in disk
void copyContent(char* p, char* f, int LC, int fileSize) {

    int remain_bytes = fileSize;
    int entry = LC;
    int counter = 0;

    while(remain_bytes > 0) {
        if (remain_bytes > 512) { // write in 512 bytes
            // memcpy(p + (entry+31) * BYTES_PER_SECTOR, f + counter*512, 512); 
            for(int i = 0; i < 512; i++) {
                p[(entry + 31)*512 + i] = f[counter*512 + i];
            }
        } else { // write in the remain bytes
            //memcpy(p + (entry+31) * BYTES_PER_SECTOR, f + counter*512, remain_bytes); 
            updateFATValue(p + BYTES_PER_SECTOR, 0xfff, entry);
            for(int i = 0; i < remain_bytes; i++) {
                p[(entry + 31)*512 +i] = f[counter*512 + i];
            }
            return;
        }
        // update the FAT table and find the next free sector
        int freeSector = getFreeSector(p + BYTES_PER_SECTOR);
        updateFATValue(p + BYTES_PER_SECTOR, entry, freeSector);
        entry = freeSector;
        remain_bytes -= 512;
        counter++;
    }
}

// copy the file information from the linux file to the disk 
// starting from the subdirectory's address, finding a free entry
int copyFileHead(char* p, char* f,char* fileName, int logical_cluster, struct stat fileInfo){

    // set the position according to the logical cluster
    // if logical cluster == 0, move to the root directory
    // if logical cluster != 0, get the adress to its physical cluster
    int position;
    if(logical_cluster == 0) {
        position = 19 * BYTES_PER_SECTOR;
    } else {
        position = (logical_cluster +31) * BYTES_PER_SECTOR;
    }

    // move forward for 32 bytes if the entry is not free
    while(p[position] != 0x00) {
        position += 0x20;
    }

    // separate the filename and extention
    char* name = calloc(1,sizeof(char) * 8);
    char* ext = calloc(1,sizeof(char) * 3);
    name = strtok(fileName,".");
    ext = strtok(NULL,".");

    // get the last write time and access date from the input file 
    time_t writeTime = fileInfo.st_ctime;
    struct tm* time = localtime(&writeTime);

    time_t accessDate = fileInfo.st_atime;
    struct tm* aDate = localtime(&accessDate);

    int aYear = ((aDate->tm_year + 1900 - 1980) << 9) & 0xFE00;
    int aMonth = (aDate->tm_mon << 5) & 0x01E0;
    int aDay = aDate->tm_mday & 0x001F;

    int year = ((time->tm_year + 1900 - 1980) << 9) & 0xFE00;
    int month = (time->tm_mon << 5) & 0x01E0;
    int day = time->tm_mday & 0x001F;
    int hour = (time->tm_hour<< 11) & 0xF800;
    int min = (time->tm_min << 5) & 0x07E0;
    int sec = time->tm_sec & 0x001F;

    // Get the file size of the the input file
    int fileSize = fileInfo.st_size;

    // Get the first free logical cluster from FAT table
    int firstLogicalCluster = getFreeSector(p + BYTES_PER_SECTOR);

    for(int i = 0; i < 8; i++) {
        p[position + i] = name[i];
    }
    for(int i = 0; i < 3; i++) {
        p[position + i + 8] = ext[i];
    }

    p[position + 11] = 0x00; // Attribute
    p[position + 14] = (((hour | min | sec) & 0xFF)); // Creation time, here is same as write time
    p[position + 15] = (((hour | min | sec) >> 8) & 0xFF);
    p[position + 16] = ((year | month | day) & 0xFF); // Creation date, here is same as write date
    p[position + 17] = (((year | month | day) >> 8) & 0xFF);
    p[position + 18] = ((aYear | aMonth | aDay) & 0xFF); // Last Access Date
    p[position + 19] = (((aYear | aMonth | aDay) >> 8) & 0xFF);
    p[position + 22] = ((hour | min | sec) & 0xFF); // Last write time
    p[position + 23] = (((hour | min | sec) >> 8) & 0xFF);
    p[position + 24] = ((year | month | day) & 0xFF); // Last write date
    p[position + 25] = (((year | month | day) >> 8) & 0xFF);
    p[position + 26] = (firstLogicalCluster & 0xFF); // First logical cluster of the file
    p[position + 27] = ((firstLogicalCluster >> 8) & 0xFF); 
    p[position + 28] = (fileSize & 0xFF); // File Size
    p[position + 29] = ((fileSize >> 8) & 0xFF); 
    p[position + 30] = ((fileSize >> 16) & 0xFF); 
    p[position + 31] = ((fileSize >> 24) & 0xFF); 

    return firstLogicalCluster;
}

// Separate the path and fileName from the inserted command
void getPathAndFile(char* insert, char* path, char* file){

    char* tempArray[10];
    char* token = strtok(insert,"/");
    int index = 0;

    // use strtok to separate the names with "/" restriction
    // and the store them into an array temp
    do {
        tempArray[index++] = token;
        token = strtok(NULL,"/");
    } while (token != NULL);

    // Move the names into their pointer in a nice format
    strcpy(file, tempArray[index - 1]);
    for(int i = 0; i < index - 1; i++){
        strcat(path, "/");
        strcat(path,tempArray[i]);
    }
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Error: input is invalid\n");
        exit(1);
    }

	int fd;
	struct stat sb;

    // open the disk and map the memory
	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map disk memory\n");
		exit(1);
	}

    // declare and get path name and file name 
    char* path = malloc(sizeof(char));
    char* file = malloc(sizeof(char));
    getPathAndFile(argv[2],path, file);

    // open the file to be copied and map the memory
    struct stat sbForFile;
    int fileToCopy = open(file,O_RDWR);
    fstat(fileToCopy,&sbForFile);

    char * f = mmap(NULL, sbForFile.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileToCopy, 0);
    if (f == MAP_FAILED) {
		printf("Error: failed to map file memory\n");
		exit(1);
	}
    // printf("the file size is %lu\n",(uint64_t)sbForFile.st_size);

    int freeSize = getFreeSize(p);
    if(freeSize < sbForFile.st_size) {
        printf("Error: there is no enough memory for the new file\n");
        exit(1);
    }

    // switch the file and path to upper case
    toUpperCase(file);
    toUpperCase(path);

    // if there is a subdirectory and does not have same file name
    // return the logical cluster of that subdirectory 
    int path_LC = searchFile(p, path, file);
    if (path_LC < 0) {
        printf("Error: the directory is not found\n");
        exit(1);
    }

    // update the file information to the destination directory and get the logical cluster for the file content
    int contentPos = copyFileHead(p,f,file,path_LC, sbForFile);

    // copy file content from f to p
    copyContent(p,f,contentPos,sbForFile.st_size);

    munmap(f,sbForFile.st_size);
	munmap(p, sb.st_size); // the modifed the memory data would be mapped to the disk image
    close(fileToCopy);
	close(fd);
	return 0;
}
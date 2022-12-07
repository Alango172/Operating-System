/*
    Name: Franco Cai
    vId: V00940471
    Subject: CSC360
    Assignment 3 part 3
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

#define BYTES_PER_SECTOR 512
#define POSSIBLE_DIRECTORY_ENTRIES 224 //total possible entries of root directory
#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

int firstLogicalCluster; // A global variable for the first logical cluster if the file is found

/*
    The helper functions of pringting the list of directories
*/

// Get the logical cluster of this directory entry 
int getLogicalCluster(char* p) {
    return (p[26] & 0x00FF) + ((p[27] & 0x00FF)<<8);    
}

int getFileSize(char* p) {
    int fileSize = (p[28] & 0xff) + ((p[29] & 0xff) << 8) + ((p[30] & 0xff) << 16) + ((p[31] & 0xff) << 24);
    return fileSize;
}

char* getFileName(char* p) {
    char* fileName = malloc(sizeof(char));
    char* extention = malloc(sizeof(char));

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

void toUpperCase(char* name) {
    while (*name) {
    *name = toupper((unsigned char) *name);
    name++;
  }
}

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

// int searchSubDir(char* p, char* fileName, int LC) {

//     int basePos = (LC+ 31) * BYTES_PER_SECTOR;
//     int firstBit = p[basePos];
//     int counter = 0;
//     int subDirRecord[POSSIBLE_DIRECTORY_ENTRIES];
//     int recordCount = 0;

//     while(firstBit != 0x00 && counter < 16){

//         counter++;
//         firstBit = p[basePos + counter*32];
//         if(firstBit == 0xE5 || (p[basePos + counter*32+11] & 0x08) == 0x08 || firstBit == '.' || getLogicalCluster(p+ basePos + counter*32) < 2) {
//             continue;
//         } 
//         if(p[basePos+ counter*32 + 11] == 0x10) {
//             subDirRecord[recordCount++] = counter;
//         } else {
//             char* fileNameDisk = getFileName(p + basePos + counter*32);
//             if(strcmp(fileNameDisk, fileName) == 0) {
//                 firstLogicalCluster = getLogicalCluster(p+ basePos + counter*32);
//                 return getFileSize(p + basePos + counter*32);
//             }
//         }
//     }

//     // A recursive call for all the subdirectories
//     for(int i = 0; i < recordCount; i++) {
//         counter = subDirRecord[i];
//         int logical_cluster = getLogicalCluster(p + basePos + counter*32);  
//         return searchSubDir(p, fileName, logical_cluster);
//     }
    
//     return -1;
// }

int searchFile(char* p, char* fileName) {
    
    int basePos = BYTES_PER_SECTOR * 19;
    int counter = -1;
    int firstBit = p[basePos];

    while(firstBit != 0x00 && counter < POSSIBLE_DIRECTORY_ENTRIES){

        counter++;
        firstBit = p[basePos + counter*32 + 32];
        int check = getLogicalCluster(p+ basePos + counter*32);
        if(firstBit == 0xE5 || p[basePos + counter*32+11] == 0x08 ||check == 0 || check == 1) {
            continue;
        } 
    
        if(p[basePos+ counter*32 + 11] == 0x10) {
            continue;
        } else {
            char* fileNameDisk = getFileName(p + basePos + counter*32);
            if(strcmp(fileNameDisk, fileName) == 0) {
                firstLogicalCluster = getLogicalCluster(p+ basePos + counter*32);
                return getFileSize(p + basePos + counter*32);
            }
        }
    }

    // If there is a subdirecory and the file is not found, search into the subdir
    // for(int i = 0; i < recordCount; i++) {
    //     counter = subDirRecord[i];
    //     int logical_cluster = getLogicalCluster(p + basePos + counter*32);
    //     return searchSubDir(p, fileName, logical_cluster);
    // }

    return -1;
}

void copyToSys(char* p, int filesize, char* fileName) {

    int remain_bytes = filesize;
    int entry = firstLogicalCluster;

    FILE* newFile = fopen(fileName, "w");

    while(remain_bytes > 0) {

        if (remain_bytes >= 512) { 
            fwrite(p + (entry + 31) * BYTES_PER_SECTOR, sizeof(char), 512, newFile);
        } else {
            fwrite(p + (entry + 31) * BYTES_PER_SECTOR, sizeof(char), remain_bytes, newFile);
        }
        entry = getFatValue(entry, p + BYTES_PER_SECTOR);
        remain_bytes -= 512;
    }

    fclose(newFile);
    return;

}

int main(int argc, char *argv[]) {
	int fd;
	struct stat sb;

    if(argc != 3) {
        printf("Error: input is invalid\n");
        exit(1);
    }

	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    char* fileName = malloc(sizeof(char));
    fileName = argv[2];
    
    // Change the fileName to all upper case
    toUpperCase(fileName);

    // If the file exist in the disk, return the fileSize of the file founded
    int exist_filesize = searchFile(p, fileName);
    if (exist_filesize == -1) {
        printf("Error: File not found.\n");
        exit(1);
    }

    // Call the function to create a file and copy content from the disk
    copyToSys(p, exist_filesize,fileName);

	munmap(p, sb.st_size); // the modifed the memory data would be mapped to the disk image
	close(fd);
	return 0;
}
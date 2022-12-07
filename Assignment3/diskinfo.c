/*
    Name: Franco Cai
    vId: V00940471
    Subject: CSC360
    Assignment 3 part 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BYTES_PER_SECTOR 512

/*
    The helper Function for getting each information needed
*/

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

void getOSName (char* name, char* p){
    for(int i = 0; i < 8;i++) {
        name[i] = p[i+3];
    }
}

void getDiskLabel(char* label, char* p) {
    for(int i = 0; i < 11; i++){
        label[i] = p[i+43];
    }
    if(label[0] == ' ') {
        p += BYTES_PER_SECTOR * 19;
        for(int i = 0; i < 14 * 16; i++, p += 32) {
            if(p[11] == 0x08){ // Check if the volume label is set to 1
                for(int j = 0; j < 8; j++) { 
                    label[j] = p[j]; // Get the file name of this label
                }
            }
        }        
    }
}

int getTotalSector(char* p) {
    return p[19] + (p[20] << 8);
}

int getSectorFat(char* p) {
    return p[22] + (p[23] << 8);
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

int getFileNum(char* p) {
	p += BYTES_PER_SECTOR * 19; // get to the root directory
	int count = 0; 
    // traverse the root directory and skip counting files if there directory entry for following conditions
    for(int i = 0; i < 14*16; i++, p += 32) {
        if(p[11] == 0x0f) {
            continue;
        } else if(((p[0] & 0xff) == 0xE5) || ((p[0] & 0xff) == 0x00)) {
            continue;
        } else if ((p[26]+(p[27] << 8)) < 2) {
            continue;
        } else if (p[11] == 0x08) {
            continue;
        }
        count++;
    }
	return count;
}

int main(int argc, char *argv[]) {
	int fd;
	struct stat sb;

	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    // Initialize the variables for the diskinfo
    char* OSName = malloc(sizeof(char));
    char* diskLabel = malloc(sizeof(char));
    getOSName(OSName, p);
    getDiskLabel(diskLabel, p);

    int totalSize = getTotalSector(p) * BYTES_PER_SECTOR;
    int sectorFAT = getSectorFat(p);
    int freeSize = getFreeSize(p);
    int nFAT = p[16];
    int fileNum = getFileNum(p);

    printf("OS Name: %s\n", OSName); 
    printf("Label of the disk: %s\n", diskLabel);
    printf("Total size of the disk: %d bytes\n", totalSize);
    printf("Free size of the disk: %d bytes\n\n", freeSize);
    printf("=================================\n");
    printf("The number of files in the image: %d\n\n", fileNum);
    printf("=================================\n");
    printf("Number of fat copies: %d\n", nFAT);
    printf("Sectors per FAT: %d\n", sectorFAT);
	
	munmap(p, sb.st_size); // the modifed the memory data would be mapped to the disk image
	close(fd);
	return 0;
}

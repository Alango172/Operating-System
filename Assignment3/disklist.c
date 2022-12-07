/*
    Name: Franco Cai
    vId: V00940471
    Subject: CSC360
    Assignment 3 part 2
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

#define BYTES_PER_SECTOR 512
#define POSSIBLE_DIRECTORY_ENTRIES 224 //total possible entries of root directory
#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

void printList(char* p);
void printFile(char* p);
void printDir(char* p);

/*
    The helper functions of pringting the list of directories
*/

void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } 
    while (*s++ = *d++);
}

void print_date_time(char * directory_entry_startPos){
	
	int time, date;
	int hours, minutes, day, month, year;
	
	time = *(unsigned short *)(directory_entry_startPos + timeOffset);
	date = *(unsigned short *)(directory_entry_startPos + dateOffset);
	
	//the year is stored as a value since 1980
	//the year is stored in the high seven bits
	year = ((date & 0xFE00) >> 9) + 1980;
	//the month is stored in the middle four bits
	month = (date & 0x1E0) >> 5;
	//the day is stored in the low five bits
	day = (date & 0x1F);
	
	printf("%d-%02d-%02d ", year, month, day);
	//the hours are stored in the high five bits
	hours = (time & 0xF800) >> 11;
	//the minutes are stored in the middle 6 bits
	minutes = (time & 0x7E0) >> 5;
	
	printf("%02d:%02d\n", hours, minutes);
	
}

// Get the logical cluster of this directory entry
int getLogicalCluster(char* p) {
    return (p[26] & 0x00FF) + ((p[27] & 0x00FF)<<8);
}

// A function for recursive call of subdirectory
void printSubDir(char* p,int logicalCluster, char* dirName) {

    int basePos = (logicalCluster+ 31) * BYTES_PER_SECTOR;
    int firstBit = p[basePos];
    int counter = 0;
    int subDirRecord[POSSIBLE_DIRECTORY_ENTRIES];
    int recordCount = 0;
    char* subDirName = malloc(sizeof(char));

    while(firstBit != 0x00 && counter < 16){

        counter++;
        firstBit = p[basePos + counter*32];
        int check = getLogicalCluster(p+ basePos + counter*32);
        if(firstBit == 0xE5 || (p[basePos + counter*32+11] & 0x08) == 0x08 || firstBit == '.' || check == 1 || check == 0) {
            continue;
        } 
        if((p[basePos+ counter*32 + 11] &0x10) == 0x10) {
            subDirRecord[recordCount++] = counter;
            printDir(p + basePos + counter*32);
        } else {
            printFile(p + basePos + counter*32);
        }
    }

    for(int i = 0; i < recordCount; i++) {

        counter = subDirRecord[i];
        for(int j = 0; j < 8; j++) {
            if (p[basePos + counter* 32 +j] == ' ') {
                continue;
            }
            subDirName[j] = p[basePos + counter* 32 + j];
        }
        // create a new string that stores the previous path name in format
        char* previousPath = calloc(1, sizeof(char)*9+strlen(dirName));
        strcat(previousPath, dirName);
        strcat(previousPath,"/");
        strcat(previousPath,subDirName);
        
        int logical_cluster = getLogicalCluster(p + basePos + counter*32);
        printf("/%s\n==================\n",previousPath);
        printSubDir(p,logical_cluster,previousPath);
    }
    
}

void printDir(char* p) {

    char type = 'D';
    int fileSize = 0x00000000;
    char* fileName = calloc(1,sizeof(char)*8);

    memcpy(fileName, p, 8);
    remove_spaces(fileName);

    printf("%c %10d %20s ", type, fileSize, fileName);
    print_date_time(p);

    free(fileName);

}

void printFile(char* p) {

    char type = 'F';
    int fileSize;
    char* fileName = calloc(1,sizeof(char) *(8+1+3));
    char* extention = calloc(1,sizeof(char)*3);

    memcpy(fileName, p, 8);
    remove_spaces(fileName);
    for(int j = 0; j < 3; j++) {
        extention[j] = p[j+8];
    }
    remove_spaces(extention);

    // combine the file name and the extention
    strcat(fileName,".");
    strcat(fileName, extention);

    fileSize = (p[28] & 0xff) + ((p[29] & 0xff) << 8) + ((p[30] & 0xff) << 16) + ((p[31] & 0xff) << 24);

    printf("%c %10u %20s ", type, fileSize, fileName);
    print_date_time(p);
    free(fileName);
    free(extention);
}

void printList(char* p) {

    int basePos = BYTES_PER_SECTOR * 19;
    int counter = -1;
    int firstBit = p[basePos];
    int subDirRecord[POSSIBLE_DIRECTORY_ENTRIES];
    int recordCount = 0;
    char* subDirName = malloc(sizeof(char));

    while(firstBit != 0x00 && counter < POSSIBLE_DIRECTORY_ENTRIES){

        counter++;
        firstBit = p[basePos + counter*32+32];
        int check = getLogicalCluster(p+ basePos + counter*32);
        if(firstBit == 0xE5 || p[basePos + counter*32+11] == 0x08 || check == 1 || check == 0) {
            continue;
        }
        if((p[basePos+ counter*32 + 11] &0x10) == 0x10) {
            subDirRecord[recordCount++] = counter;
            printDir(p + basePos + counter*32);
        } else {
            printFile(p + basePos + counter*32);
        }
    }
    
    for(int i = 0; i < recordCount; i++) {
        counter = subDirRecord[i];
        for(int j = 0; j < 8; j++) {
            if (p[basePos + counter* 32 +j] == ' ') {
                continue;
            }
            subDirName[j] = p[basePos + counter* 32 + j];
        }
        int logical_cluster = getLogicalCluster(p + basePos + counter*32);
        printf("/%s\n==================\n",subDirName);
        printSubDir(p,logical_cluster,subDirName);
    }
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
    
    printf("root\n======================\n");
    printList(p);
	
	munmap(p, sb.st_size); // the modifed the memory data would be mapped to the disk image
	close(fd);
	return 0;
}
/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: createDirectory.h
*
* Description:: Interface for the directory creation 
                and managment
*
**************************************************************/

#ifndef CREATE_DIRECTORY_H
#define CREATE_DIRECTORY_H

#include "fsLow.h"
#include <stdint.h> 
#include <stdbool.h> 
#include <time.h>

#define MAX_FILENAME_LENGTH 100
#define DIRECTORY_ENTRY_SIZE sizeof(DirectoryEntry)

typedef struct DirectoryEntry {
char fileName[MAX_FILENAME_LENGTH]; // name of the file or directory
uint16_t permissions;  // bit field to describe a file’s permissions 
uint32_t fileSize; 	// size of file in Bytes 
uint32_t startBlock;    // starting block location for this file
time_t createdTime; 	// file creation timestamp
time_t lastModified;   // last time modified timestamp
time_t lastAccessed;   // last time accessed timestamp	
bool isDir; 		// flag to tell if this entry is a directory or a file 
bool inUse; // ALTERNATIVE FOR NOW TO TRACK THE ENTRY STATUS
}DirectoryEntry;

uint64_t createDirectory(int initialNumEntries, DirectoryEntry * parent);

#endif
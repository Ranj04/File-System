/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: createDirectory.c
*
* Description:: Implementation for creating the root and 
                other directories
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "createDirectory.h"
#include "freeSpace.h"
#include "vcb.h"

uint64_t createDirectory(int initialNumEntries, DirectoryEntry * parent)
{
    int initialBytesNeeded = initialNumEntries * sizeof(DirectoryEntry);

    int blocksNeeded = (initialBytesNeeded + BLOCK_SIZE -1) / BLOCK_SIZE;

    int actualBytesNeeded = blocksNeeded * BLOCK_SIZE;

    int actualNumEntries = actualBytesNeeded / sizeof(DirectoryEntry);

    DirectoryEntry * newDirectory = malloc(actualBytesNeeded);

    if (newDirectory == NULL) 
    {
        fprintf(stderr, "Error: Failed to allocate memory for directory\n");
        return 0;
    }

    //Implemented once we get the free space functions needed.
    uint64_t startBlock = allocateBlocks(blocksNeeded);

    for(int i = 0; i < actualNumEntries; i++)
    {
        newDirectory[i].inUse = false;
    }

     // Set up "." directory entry
    time_t currentTime = time(NULL);

    strcpy(newDirectory[0].fileName, ".");
    newDirectory[0].isDir = true;
    newDirectory[0].fileSize = actualBytesNeeded;
    newDirectory[0].startBlock = startBlock;
    newDirectory[0].inUse = true;

    //Time related fields
    newDirectory[0].createdTime = currentTime;
    newDirectory[0].lastModified = currentTime;
    newDirectory[0].lastAccessed = currentTime;
    
    // Set up ".." directory entry
    strcpy(newDirectory[1].fileName, "..");
    
    // If parent is NULL, this is the root directory
    if (parent == NULL) {
        // For root, ".." points to itself
        newDirectory[1].isDir = true;
        newDirectory[1].fileSize = actualBytesNeeded;
        newDirectory[1].startBlock = startBlock;
        newDirectory[1].inUse = true;
        
    } else {
        // For non-root directories, ".." points to parent
        newDirectory[1].isDir = true;
        newDirectory[1].fileSize= parent[0].fileSize;
        newDirectory[1].startBlock = parent[0].startBlock;
        newDirectory[1].inUse = true;
    }
    
    //Time related fields
    newDirectory[1].createdTime = currentTime;
    newDirectory[1].lastModified = currentTime;
    newDirectory[1].lastAccessed = currentTime;

    uint64_t blocksWritten = LBAwrite(newDirectory, blocksNeeded, startBlock);

    if (blocksWritten != blocksNeeded) 
    {
        fprintf(stderr, "Error: Failed to write directory (wrote %d of %d blocks)\n", 
            blocksWritten, blocksNeeded);
            
        free(newDirectory);
        return 0;
    }
    
    // Free the memory
    free(newDirectory);
    
    // Return the starting block
    return startBlock;
}

/*
    EXAMPLE INIT:

    For root: createDirectory(50, NULL);
    For any directory: createDirectory(50, parent);

*/


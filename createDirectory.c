/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Julia Bui & Eugenio Ramirez
* Student IDs::
* GitHub-Name:: Tybo2020
* Group-Name:: The Ducklings
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

uint64_t createDirectory(int initialNumEntries, DirectoryEntry* parent) {   
    // Calculate space needed
    int initialBytesNeeded = initialNumEntries * sizeof(DirectoryEntry);
    printf("Size of DirectoryEntry: %lu bytes\n", sizeof(DirectoryEntry));
    int blocksNeeded = (initialBytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int actualBytesNeeded = blocksNeeded * BLOCK_SIZE;
   
    DirectoryEntry* newDirectory = malloc(actualBytesNeeded);
    if (newDirectory == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for directory\n");
        return 0;
    }
    
    // Initialize directory memory to zeros
    memset(newDirectory, 0, actualBytesNeeded);
    
    // Allocate blocks for the directory
    uint64_t startBlock = allocateBlocks(blocksNeeded);
    if (startBlock <= 0) {
        fprintf(stderr, "Error: Failed to allocate blocks for directory\n");
        free(newDirectory);
        return 0;
    }
    
    // Set up "." directory entry
    time_t currentTime = time(NULL);
    strncpy(newDirectory[0].fileName, ".", sizeof(newDirectory[0].fileName) - 1);
    newDirectory[0].isDir = true;
    newDirectory[0].fileSize = actualBytesNeeded;
    newDirectory[0].startBlock = startBlock;
    newDirectory[0].inUse = true;
    newDirectory[0].createdTime = currentTime;
    newDirectory[0].lastModified = currentTime;
    newDirectory[0].lastAccessed = currentTime;
    
    // Set up ".." directory entry
    strncpy(newDirectory[1].fileName, "..", sizeof(newDirectory[1].fileName) - 1);
    
    // If parent is NULL, this is the root directory
    if (parent == NULL) {
        // For root, ".." points to itself
        newDirectory[1].isDir = true;
        newDirectory[1].fileSize = actualBytesNeeded;
        newDirectory[1].startBlock = startBlock;
    } else {
        // For non-root directories, ".." points to parent
        newDirectory[1].isDir = parent[0].isDir;
        newDirectory[1].fileSize = parent[0].fileSize;
        newDirectory[1].startBlock = parent[0].startBlock;
    }
    
    newDirectory[1].inUse = true;
    newDirectory[1].createdTime = currentTime;
    newDirectory[1].lastModified = currentTime;
    newDirectory[1].lastAccessed = currentTime; 
    
    // Mark remaining entries as not in use
    for (int i = 2; i < initialNumEntries; i++) {
        newDirectory[i].inUse = false;
    }
    
    // Write the directory to disk - handle non-contiguous blocks
    uint64_t blocksWritten = 0;
    int currentBlock = startBlock;
    int bytesRemaining = actualBytesNeeded;
    int offset = 0;
    
    while (bytesRemaining > 0 && currentBlock != BLOCK_RESERVED) {
        int bytesToWrite = (bytesRemaining < BLOCK_SIZE) ? bytesRemaining : BLOCK_SIZE;
        
        // Write a single block
        if (LBAwrite(((char*)newDirectory) + offset, 1, currentBlock) != 1) {
            fprintf(stderr, "Error: Failed to write directory block %lu\n", (unsigned long)currentBlock);
            releaseBlocks(startBlock, blocksNeeded);
            free(newDirectory);
            return 0;
        }
        
        blocksWritten++;
        bytesRemaining -= BLOCK_SIZE; 
        offset += BLOCK_SIZE;         
        
        // Get next block in chain
        if (bytesRemaining > 0) {
            currentBlock = readFATEntry(currentBlock);
            
            // Check for invalid next block
            if (currentBlock <= 0 && bytesRemaining > 0) {
                fprintf(stderr, "Error: Unexpected end of block chain\n");
                releaseBlocks(startBlock, blocksNeeded);
                free(newDirectory);
                return 0;
            }
        }
    }
    
    if (bytesRemaining > 0) {
        fprintf(stderr, "Error: Failed to write directory (wrote %lu of %d blocks)\n", 
               blocksWritten, blocksNeeded);
        releaseBlocks(startBlock, blocksNeeded);
        free(newDirectory);
        return 0;
    }
    
    //free(newDirectory);
    
    // Return the starting block
    return startBlock;
}


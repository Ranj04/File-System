/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: free_space.h
*
* Description:: Interface for free space management
*
**************************************************************/

#ifndef FREE_SPACE_H
#define FREE_SPACE_H

#include <stdint.h>
#include <stdbool.h>
#include "fsLow.h"

typedef uint32_t FATEntry; 

void initFreeSpace(int blockCount, int blockSize);
int allocateBlocks(int numOfBlocks);
int extendChain(int headOfChain, int amountToChange);
int releaseBlocks(int location, int numOfBlocks);

// Helper Functions 
bool isBlockFree(int blockNum);

FATEntry readFATEntry(int blockNum);
bool writeFATEntry(int blockNum, FATEntry entry);
uint64_t getFATEntryPos(int blockNum);

extern struct VolumeControlBlock* vcb;

#endif
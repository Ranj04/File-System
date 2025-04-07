/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: vcb.h
*
* Description:: Interface for the volume control block
*
**************************************************************/

#ifndef VCB_H
#define VCB_H

#include <stdint.h>
#ifndef VCB_H
#define VCB_H

#define BLOCK_SIZE 512
#define MAGIC_NUMBER 0xDEADBEEF

typedef struct VolumeControlBlock {
    unsigned long signature;       // Used to check formatting
    uint32_t volumeSize;           // Total number of blocks
    uint32_t totalBlocks;          // Total usable blocks
    uint32_t rootDirBlock;         // Starting block of root directory
    uint32_t freeSpaceStartBlock;  // (Optional, for FAT start)
    uint32_t freeSpaceHead;        // Head of free space chain
    uint32_t freeSpaceSize;        // Size of the free space region
} VolumeControlBlock;

extern VolumeControlBlock* vcb;

#endif



void initializeVCB(VolumeControlBlock *vcbPointer, unsigned long signature);

#endif
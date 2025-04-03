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

#include <stdint.h>

#define BLOCK_SIZE 512

typedef struct VolumeControlBlock {
	unsigned long signature; 	    // Signature of the file system
	uint32_t volumeSize; 		    // Total volume size in blocks
	uint32_t totalBlocks; 		    // Total number of blocks in volume
	uint32_t rootDirBlock; 	        // Location of the root directory
	uint32_t freeSpaceStartBlock;	// Where the FAT free space table starts/
	uint32_t freeSpaceHead;	        // Pointer to first free space block 
	uint32_t freeSpaceSize;	        // Size of free space tracking table in blocks
}VolumeControlBlock; 

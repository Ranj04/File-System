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
#include "freeSpace.h"

void initFreeSpace(int blockCount, int blockSize){
    int entriesPerBlock = blockSize / sizeof(FATEntry);

    int blocksNeeded = (blockCount + entriesPerBlock - 1) / entriesPerBlock;

    // Allocate buffer for one FAT entry block
    FATEntry* fatBuffer = (FATEntry*)malloc(blockSize);

    vcb->freeSpaceStartBlock = 1; 
    vcb->freeSpaceSize = blocksNeeded;
    
    // Find first block after VCB and FAT
    int firstFreeBlock = vcb->freeSpaceStartBlock + blocksNeeded;

    // Mark VCB and FAT as reserved 
    // Create linked list of free blocks
    for (int blockNum = 0; blockNum < blockCount; blockNum += entriesPerBlock) {
        // Clear the buffer
        memset(fatBuffer, 0, blockSize);
        
        // Process entries in this block
        for (int i = 0; i < entriesPerBlock && (blockNum + i) < blockCount; i++) {
            int currentBlock = blockNum + i;
            
            if (currentBlock < firstFreeBlock) {
                // Mark system blocks as reserved (VCB and FAT) 
                fatBuffer[i] = BLOCK_RESERVED;
            }
            else if (currentBlock == blockCount - 1) {
                // Last block in volume points to nothing, end of free list
                fatBuffer[i] = BLOCK_FREE;
            }
            else {
                // Each free block will have a pointer to the next free one
                fatBuffer[i] = currentBlock + 1;
            }
        }
        
        // Write FAT block to disk
        int fatBlockIndex = blockNum / entriesPerBlock;
        uint64_t lbaPosition = vcb->freeSpaceStartBlock + fatBlockIndex;
        
        if (LBAwrite(fatBuffer, 1, lbaPosition) != 1) {
            printf("Error: Failed to write FAT block %d\n", fatBlockIndex);
            free(fatBuffer);
            return;
        }
    }
    
    vcb->freeSpaceHead = firstFreeBlock;

    free(fatBuffer);

    // Write the updated VCB to disk
    LBAwrite(vcb, 1, 0);

}
int allocateBlocks(int numOfBlocks) {
    if (numOfBlocks <= 0) {
        return -1;
    }
    
    // Initialize tracking variables
    int startBlock = -1;       
    int currentBlock = -1;     
    int prevBlock = -1;        
    int remainingBlocks = numOfBlocks;
    int blockCounter = 0;
    
    currentBlock = vcb->freeSpaceHead;
    
    // Check if we have a valid starting point
    if (currentBlock <= 0) {
        return -1; // No free blocks available
    }
    
    // Process each block we need to allocate
    while (remainingBlocks > 0 && currentBlock > 0) {
        // Verify this block is actually free
        if (!isBlockFree(currentBlock)) {
            // Move to next block in the FAT
            currentBlock = readFATEntry(currentBlock);
            continue;
        }
        
        // Get the next free block before we modify this entry
        int nextFreeBlock = readFATEntry(currentBlock);
        
        // If this is our first block, save it as the start block
        if (startBlock == -1) {
            startBlock = currentBlock;
        }
        
        // If we have a previous block, link it to this one
        if (prevBlock != -1) {
            writeFATEntry(prevBlock, currentBlock);
        }
        
        // Update tracking variables
        prevBlock = currentBlock;
        currentBlock = nextFreeBlock;
        remainingBlocks--;
        blockCounter++;
        
        // Update free space head if we're using the current head
        if (blockCounter == 1) {
            vcb->freeSpaceHead = nextFreeBlock;
        }
    }
    
    // Check if we got all the blocks we needed
    if (remainingBlocks > 0) {
        // Not enough free blocks available
        return -1;
    }
    
    // Mark the last block as end of chain
    writeFATEntry(prevBlock, BLOCK_RESERVED);
    
    // Update VCB on disk
    LBAwrite(vcb, 1, 0);
    
    return startBlock;
}
int extendChain(int headOfChain, int amountToChange){

}
int releaseBlocks(int location, int numOfBlocks){

}

bool isBlockFree(int blockNum) {
    // Ensure the block number is valid
    if (blockNum <= 0 || blockNum >= vcb->totalBlocks) {
        return false;
    }
    
    FATEntry entry = readFATEntry(blockNum);
    
    // Check if the entry indicates the block is free
    return (entry == BLOCK_FREE);
}

// Retrieves the next block in the chain
FATEntry readFATEntry(int blockNum) {
    // Validate block number
    if (blockNum < 0 || blockNum >= vcb->totalBlocks) {
        return BLOCK_RESERVED; // Return reserved as an error indicator
    }
    
    // Calculate which FAT block contains this entry
    uint64_t fatPosition = getFATEntryPos(blockNum);
    int fatBlockNum = fatPosition / BLOCK_SIZE;
    int fatOffset = fatPosition % BLOCK_SIZE;
    
    // Allocate buffer for reading the FAT block
    FATEntry* fatBuffer = (FATEntry*)malloc(BLOCK_SIZE);
    if (fatBuffer == NULL) {
        return BLOCK_RESERVED;
    }
    
    // Read the FAT block from disk
    uint64_t lbaPosition = vcb->freeSpaceStartBlock + fatBlockNum;
    if (LBAread(fatBuffer, 1, lbaPosition) != 1) {
        free(fatBuffer);
        return BLOCK_RESERVED;
    }
    
    // Calculate the index within the buffer
    int entryIndex = fatOffset / sizeof(FATEntry);
    
    FATEntry entry = fatBuffer[entryIndex];
    free(fatBuffer);
    
    return entry;
}  

// Used to update block pointers when allocating, freeing, or extending a chain
bool writeFATEntry(int blockNum, FATEntry entry) {
    // Validate block number
    if (blockNum < 0 || blockNum >= vcb->totalBlocks) {
        return false;
    }
    
    // Calculate which FAT block contains this entry
    uint64_t fatPosition = getFATEntryPos(blockNum);
    int fatBlockNum = fatPosition / BLOCK_SIZE;
    int fatOffset = fatPosition % BLOCK_SIZE;
    
    // Allocate buffer for reading/writing the FAT block
    FATEntry* fatBuffer = (FATEntry*)malloc(BLOCK_SIZE);
    if (fatBuffer == NULL) {
        return false;
    }
    
    // Read the FAT block from disk
    uint64_t lbaPosition = vcb->freeSpaceStartBlock + fatBlockNum;
    if (LBAread(fatBuffer, 1, lbaPosition) != 1) {
        free(fatBuffer);
        return false;
    }
    
    // Calculate the index within the buffer
    int entryIndex = fatOffset / sizeof(FATEntry);
    
    // Update the entry
    fatBuffer[entryIndex] = entry;
    
    // Write the updated block back to disk
    if (LBAwrite(fatBuffer, 1, lbaPosition) != 1) {
        free(fatBuffer);
        return false;
    }
    
    // Free the buffer
    free(fatBuffer);
    
    return true;
}
uint64_t getFATEntryPos(int blockNum){

    // Calculate the byte position for this entry in the FAT
    return blockNum * sizeof(FATEntry);
}

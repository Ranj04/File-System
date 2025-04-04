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
int allocateBlocks(int numOfBlocks){

}
int extendChain(int headOfChain, int amountToChange){

}
int releaseBlocks(int location, int numOfBlocks){

}

bool isBlockFree(int blockNum){

}

// Retrieves the next block in the chain
FATEntry readFATEntry(int blockNum){

}

// Used to update block pointers when allocating, freeing, or extending a chain
bool writeFATEntry(int blockNum, FATEntry entry){

}
uint64_t getFATEntryPos(int blockNum){

}

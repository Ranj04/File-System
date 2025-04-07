/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include "fsLow.h"
#include "vcb.h"
#include "freeSpace.h"
#include "createDirectory.h"

VolumeControlBlock* vcb = NULL;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
    printf("Initializing File System with %ld blocks and block size %ld bytes\n", numberOfBlocks, blockSize);

    vcb = malloc(sizeof(VolumeControlBlock));
    if (!vcb) {
        fprintf(stderr, "VCB malloc failed\n");
        return -1;
    }

    if (LBAread(vcb, 1, 0) != 1) {
        fprintf(stderr, "Error reading block 0\n");
        return -1;
    }

    if (vcb->signature != MAGIC_NUMBER) {
        printf("Volume not formatted. Formatting now...\n");

        vcb->signature = MAGIC_NUMBER;
        vcb->volumeSize = numberOfBlocks;
        vcb->totalBlocks = numberOfBlocks;

        printf("Calling initFreeSpace...\n");
        initFreeSpace(numberOfBlocks, blockSize);

        printf("Calling createDirectory...\n");
        vcb->rootDirBlock = createDirectory(50, NULL);

        printf("Writing VCB back to disk...\n");
        if (LBAwrite(vcb, 1, 0) != 1) {
            fprintf(stderr, "Error writing VCB to disk\n");
            return -1;
        }

        printf("Volume formatted successfully!\n");
    } else {
        printf("Volume already formatted.\n");
    }

    return 0;
}

void exitFileSystem()
{
    printf("System exiting.\n");
    if (vcb) {
        free(vcb);
        vcb = NULL;
    }
}

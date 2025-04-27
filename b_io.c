/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "createDirectory.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int currentBlk; //holds the current block number
	int numBlocks; //hold how many blocks file occupies
	DirectoryEntry * de; //holds the low level systems file info
    int flags; 
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char * filename, int flags)
{
    b_io_fd returnFd;
    
    if (startup == 0) b_init();  //Initialize our system

    if (filename == NULL || flags <= 0){
        return -1; 
    }

    returnFd = b_getFCB();              // get our own file descriptor
    if (returnFd == -1){                // check for error - all used FCB's
        printf("no free slots available \n");
        return -1; 
    }

    // Duplicate filename to avoid modifying orignal 
    char *currentPathname = malloc(strlen(filename) + 1); 
    if (currentPathname == NULL){
        printf("malloc failed for pathname \n"); 
        return -1; 
    }

    strcpy(currentPathname, filename);

    // Parse pathname 
    ppinfo *info = malloc(sizeof(ppinfo));
    if (info == NULL) {
        free(currentPathname);
        fprintf(stderr, "malloc failed for path info \n");
        return -1;
    }

    int parseResult = parsePath(currentPathname, info);
    DirectoryEntry *fi = NULL;
    
    // Check if file already exists
    if (parseResult == -2 || parseResult == -1) {
            free(info);
            free(currentPathname);
            return -1;
        }

    if (info -> index >= 0){
        fi = &(info -> parent [info -> index]);

        if (fi -> isDir){
            free(info);
            free(currentPathname);
            printf("cannot open a dir \n");
            return -1;
        }

        // Handle O_TRUNC flag
        if (flags & O_TRUNC) {
            fi->fileSize = 0;
            LBAwrite(info->parent, (info->parent[0].fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE, info->parent[0].startBlock);
        }
    }

        // File does not exist
        else if (info->index == -1) {
            if (flags & O_CREAT) {
                // Find a free entry in the parent directory
                int freeIndex = -1;
                for (int i = 0; i < MAX_ENTRIES; i++) {
                    if (!info->parent[i].inUse) {
                        freeIndex = i;
                        break;
                    }
                }
                if (freeIndex == -1) {
                    free(info);
                    free(currentPathname);
                    return -1;
                }

                // Create a new file entry
                fi = &(info->parent[freeIndex]);
                memset(fi, 0, sizeof(DirectoryEntry));
                strncpy(fi->fileName, info->lastElementName, sizeof(fi->fileName) - 1);
                fi->isDir = false;
                fi->fileSize = 0;
                fi->startBlock = allocateBlocks(1);  // allocate at least 1 block
                fi->inUse = true;
                fi->createdTime = fi->lastModified = fi->lastAccessed = time(NULL);

                // Update parent directory on disk
                LBAwrite(info->parent, (info->parent[0].fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE, info->parent[0].startBlock);
            } else {
                // Cannot create file if O_CREAT is not set
                free(info);
                free(currentPathname);
                return -1;
            }
        }

       
        // Set up the FCB for the new file
        fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
        if (fcbArray[returnFd].buf == NULL) {
            free(info);
            free(currentPathname);
            fprintf(stderr, "malloc for buffer failed\n");
            return -1;
        }
        
        fcbArray[returnFd].index = 0;
        fcbArray[returnFd].buflen = 0;
        fcbArray[returnFd].currentBlk = 0;
        fcbArray[returnFd].numBlocks = 1;  // Start with one block
        fcbArray[returnFd].de = fi;
        fcbArray[returnFd].flags = flags;
    
    // Handle O_APPEND flag
    if (flags & O_APPEND) {
        fcbArray[returnFd].index = fi->fileSize % B_CHUNK_SIZE;
        fcbArray[returnFd].currentBlk = fi->fileSize / B_CHUNK_SIZE;
    }
    
    free(info);
    free(currentPathname); 
    
    return (returnFd);                  // all set
}



// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	// Check if this FCB is actually in use
    if (fcbArray[fd].buf == NULL)
    {
        return (-1);             // FCB not in use
    }
    
    // Get directory entry from FCB
    DirectoryEntry* de = fcbArray[fd].de;
    if (de == NULL)
    {
        return (-1);             // No directory entry available
    }
    
    // Calculate current absolute position and file size
    int currentPos = (fcbArray[fd].currentBlk * B_CHUNK_SIZE) + fcbArray[fd].index;
    int fileSize = de->fileSize;  // From the directory entry
    
    // Calculate new position based on whence
    int newPos = 0;
    switch (whence)
    {
        case SEEK_SET:  // From beginning of file
            newPos = offset;
            break;
            
        case SEEK_CUR:  // From current position
            newPos = currentPos + offset;
            break;
            
        case SEEK_END:  // From end of file
            newPos = fileSize + offset;  
            break;
            
        default:
            return (-1);  
    }
    
    // Validate the new position
    if (newPos < 0)
    {
        return (-1);  // Cannot seek before start of file
    }
    
    if (newPos > fileSize)
    {   
        newPos = fileSize;
    }
    
    // Calculate new block number and position within that block
    int newBlockNum = newPos / B_CHUNK_SIZE;
    int newIndex = newPos % B_CHUNK_SIZE;
    
    // If we're moving to a different block, we need to load that block
    if (newBlockNum != fcbArray[fd].currentBlk)
    {
        // Assumption: block loading/saving will be handled in b_read/b_write
        // Update the current block in the FCB
        fcbArray[fd].currentBlk = newBlockNum;
        
        // Assumption: buffer management will be handled in b_read/b_write
        // For a complete implementation, we might need to load the new block here
    }
    
    // Update the position within the buffer
    fcbArray[fd].index = newIndex;
    
    // Return the new absolute position
    return newPos;	
	
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
    if(fcbArray[fd].de == NULL){//Checks if the file is open
        return -1;
    }
    
    // How much of the buffer has been filled
    int bBytesLeft = fcbArray[fd].buflen - fcbArray[fd].index;
    // Total bytes written to file
    int bytesWritten = fcbArray[fd].currentBlk*B_CHUNK_SIZE - bBytesLeft;
    // Bytes left unwritten in file
    int fBytesLeft = fcbArray[fd].de->fileSize - bytesWritten;
    // Ensures count is never more than what is left in file
    if(count >= fBytesLeft){
        count = fBytesLeft;
    }
    // Deal with bytes left in file buffer first
    if(bBytesLeft != 0){
        // Fills file buffer
        if(count <= bBytesLeft){
            memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, count);
            fcbArray[fd].buflen -= count;
            if(fcbArray[fd].buflen == 0){
                // Writes buffer to file
                LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].de->startBlock + fcbArray[fd].currentBlk);
                fcbArray[fd].currentBlk++;
                fcbArray[fd].buflen = B_CHUNK_SIZE;
                fcbArray[fd].index = 0;
            }else{
                // Updates buffer location
                fcbArray[fd].index += count;
            }
            return count;
        }else{
            memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, bBytesLeft);
            LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].de->startBlock + fcbArray[fd].currentBlk);
            fcbArray[fd].currentBlk++;
            count -= bBytesLeft;
        }
    }
    int countBlocks = (count + (B_CHUNK_SIZE - 1)) / B_CHUNK_SIZE;
    int leftOver = 0;
    if(count % B_CHUNK_SIZE == 0){
        // Count divides perfectly
        LBAwrite(buffer, countBlocks, fcbArray[fd].de->startBlock + fcbArray[fd].currentBlk);
    }else{
        // Handles bytes that don't completely fill a block
        leftOver = countBlocks * B_CHUNK_SIZE - count;
        countBlocks--;
        LBAwrite(buffer, countBlocks, fcbArray[fd].de->startBlock + fcbArray[fd].currentBlk);
        memcpy(fcbArray[fd].buf, buffer, leftOver);
        fcbArray[fd].buflen -= leftOver;
        fcbArray[fd].index += leftOver;
    }
    return count;
	}


// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{
	int bytesRead; 				// for our reads
	int bytesReturned;			// what we will return
	int part1, part2, part3; 	// holds the three potential copy lengths 
	int numbersOfBlocksToCopy;	// holds the number of whole blocks that are needed
	int remainingBytesInBuffer;	// holds how many bytes are left in my buffer

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	if (fcbArray[fd].de == NULL) {	// File is not open for this fd
		return -1; 
	}

	// Number of bytes available to copy from buffer
	remainingBytesInBuffer = fcbArray[fd].buflen - fcbArray[fd].index;

	// Limit count to file length - i.e. handle End-Of-File 
	int amountAlreadyDelivered = (fcbArray[fd].currentBlk * B_CHUNK_SIZE) - remainingBytesInBuffer;

	if((count + amountAlreadyDelivered) > fcbArray[fd].de->fileSize) {
		count = fcbArray[fd].de->fileSize - amountAlreadyDelivered;

		if(count < 0) {
			printf("ERROR: Count: %d - Delivered: %d - CurBlk: %d - Index: %d\n", count, amountAlreadyDelivered,
					fcbArray[fd].currentBlk, fcbArray[fd].index);
		}
	}

	// Part 1 is the first copy of data which will be from the current buffer 
	// It will be the lesser of the requested amount or the number of bytes that remain in the buffer 

	if (remainingBytesInBuffer >= count) // we have enough in buffer 
		{
		part1 = count; // completely buffered (requested amount is smaller than what remains) 
		part2 = 0;
		part3 = 0; 	// Do not need anything from the "next" buffer 
		}
	else 
		{
		part1 = remainingBytesInBuffer; 	// spanning buffer (or first read) 

		//Part 1 is not enough 0 set part 3 to how much more is needed 
		part3 = count - remainingBytesInBuffer;  // How much more we still need to copy

		// The Following calculates how many 512 bytes chunks need to be copied to
		// the callers buffer from the count of what is left to copy
		numbersOfBlocksToCopy = part3/ B_CHUNK_SIZE; // This is integer math 
		part2 = numbersOfBlocksToCopy * B_CHUNK_SIZE; 

		// Reduce part 3 by the number of bytes that can be copied in chunks 
		// Part 3 at this point must be less than the block size 
		part3 = part3 - part2; // This would be equivalent to part3 % B_CHUNK_SIZE 
		}

	if(part1 > 0) 	// memcpy part 1 
		{
		memcpy (buffer, fcbArray[fd].buf + fcbArray[fd].index, part1);
		fcbArray[fd].index = fcbArray[fd].index + part1; 
		}
	
	if(part2 > 0) // blocks to copy direct to callers bufffer
		{
		// limit blocks to blocks left

		bytesRead = LBAread (buffer+part1, numbersOfBlocksToCopy, fcbArray[fd].currentBlk + fcbArray[fd].de->startBlock);

		fcbArray[fd].currentBlk += numbersOfBlocksToCopy;
		part2 = bytesRead * B_CHUNK_SIZE; // might be less if we hit end of the file 

		// Alternative version that only reads one block at a time
		/*****
		int tempPart2 = 0; 
		for (int i = 0; i < numberOfBlocksToCopy; i++){
			bytesRead = LBAread (buffer + part1 + tempPart2, 1,
								fcbArray[fd].currentBlk + fcbArray[fd].fi-> location);
			bytesRead = bytesRead * B_CHUNK_SIZE; 
			tempPart2 = tempPart2 + bytesRead; 
			++fcbArray[fd].currentBlk; 
		}
		part2 = tempPart2; 
			*/
		}
	if (part3 > 0) // We need to refill our buffer to copy more bytes to user 
		{
		// try to read B_CHUNK_SIZE bytes into our buffer 
		bytesRead = LBAread(fcbArray[fd].buf, 1, 
							fcbArray[fd].currentBlk + fcbArray[fd].de->startBlock);
		
		bytesRead = bytesRead * B_CHUNK_SIZE;

		fcbArray[fd].currentBlk += 1;
		// we just did a read into our buffer - reset the offest and buffer length. 
		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = bytesRead; // how many bytes are actually in buffer

		if (bytesRead < part3) // not even enough left to satisfy read request from caller 
			part3 = bytesRead; 

		if (part3 > 0) // memcpy bytesRead 
			{
			memcpy(buffer + part1 + part2, fcbArray[fd].buf + fcbArray[fd].index, part3);
			fcbArray[fd].index = fcbArray[fd].index + part3; // adjust index for copied bytes 
			}
		}
	bytesReturned = part1 + part2 + part3; 

	// Never do the following line. It is here to test the buffer overrun grading code 
	// buffer[bytesReturned] = '\5';
	return (bytesReturned);

	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
{
    if (startup == 0) b_init();  // Initialize if needed

    // Validates the file descriptor
    if (fd < 0 || fd >= MAXFCBS) {
        return -1; // Invalid file descriptor
    }

    // Checks if FCB is actually in use
    if (fcbArray[fd].buf == NULL) {
        return -1; // Not an open file
    }

    // If there are unwritten bytes left in the buffer, this will write them to disk
    if (fcbArray[fd].index > 0) {
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].de->startBlock + fcbArray[fd].currentBlk);
    }

    // Free the buffer
    free(fcbArray[fd].buf);
    fcbArray[fd].buf = NULL;

    // Clears the other fields to mark this FCB slot as free
    fcbArray[fd].index = 0;
    fcbArray[fd].buflen = 0;
    fcbArray[fd].currentBlk = 0;
    fcbArray[fd].numBlocks = 0;
    fcbArray[fd].de = NULL;

    return 0; // Success
}

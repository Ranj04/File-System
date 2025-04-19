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
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	
	return (returnFd);						// all set
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
		
		
	return (0); //Change this
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

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{

	}

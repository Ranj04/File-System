/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Ty Bohlander, Julia Bui, Eugenio Ramirez, Juan Ramirez
* Student IDs:: 
* GitHub-Name:: Tybo2020
* Group-Name:: The Ducklings
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: 
*
**************************************************************/
#include "mfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freeSpace.h"

char currentWorkingPath[MAXDIR_LEN] = "/";


DirectoryEntry* rootDirectory = NULL;
DirectoryEntry* currentWorkingDirectory = NULL;

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));

    if(parsePath(pathname, ppinfo) != 0){
        free(ppinfo);
        return -1;
    }

    //Check if a file/directory with the same name already exists in that location
    if(ppinfo->index != -1){
        free(ppinfo);
        return -1;
    }

    uint64_t startBlock = createDirectory(MAX_ENTRIES, ppinfo->parent);

    if(startBlock == 0){
        free(ppinfo);
        return -1;  // Failed to create directory
    }

    // Find a free entry in the parent directory for the new directory
    int freeIndex = -1;
    for(int i = 0; i < MAX_ENTRIES; i++){
        if(!ppinfo->parent[i].inUse){
            freeIndex = i;
            break;
        }
    }
    
    if(freeIndex == -1){
        // No free space in parent directory
        free(ppinfo);
        return -1;
    }

    // Initialize the new directory entry in the parent
    time_t currentTime = time(NULL);
    strncpy(ppinfo->parent[freeIndex].fileName, ppinfo->lastElementName, sizeof(ppinfo->parent[freeIndex].fileName) - 1);
    ppinfo->parent[freeIndex].isDir = true;
    ppinfo->parent[freeIndex].fileSize = MAX_ENTRIES * sizeof(DirectoryEntry);  // Approximate size
    ppinfo->parent[freeIndex].startBlock = startBlock;
    ppinfo->parent[freeIndex].inUse = true;
    ppinfo->parent[freeIndex].createdTime = currentTime;
    ppinfo->parent[freeIndex].lastModified = currentTime;
    ppinfo->parent[freeIndex].lastAccessed = currentTime;
    
    // Write the updated parent directory back to disk
    int parentBlocks = (ppinfo->parent[0].fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if(LBAwrite(ppinfo->parent, parentBlocks, ppinfo->parent[0].startBlock) != parentBlocks){
        free(ppinfo);
        return -1;  // Failed to update parent directory
    }
    
    free(ppinfo);
    return 0;

}

int fs_rmdir(const char *pathname){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));
    int firstBlock;
    int totalBlocks;

    // Checks if directory exits
    if(parsePath(pathname, ppinfo) != 0){
        free(ppinfo);
        return -1;
    }else{
        int indx = ppinfo->index;
        DirectoryEntry  *dir = &(ppinfo->parent[indx]);
        // Checks if directory really is a directory
        if(!isDEaDir(dir)){
            free(ppinfo);
            return -1;
        }else{
            // Checks if directory is empty
            for(int i = 0; i < MAX_ENTRIES; i++){
                if(dir[i].inUse == true){
                    return -1;
                }
            }
            // Removes directory by freeing the memory tied to it
            firstBlock = dir->startBlock;
            totalBlocks = (dir->fileSize + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
            releaseBlocks(firstBlock, totalBlocks);
            strcpy(dir->fileName, "NULL");
            dir->permissions = 0;
            dir->fileSize = 0;
            dir->startBlock = 0;
            dir->createdTime = 0;
            dir->lastModified = 0;
            dir->lastAccessed = 0;
            dir->isDir = NULL;
            dir->inUse = false;
            free(dir);
            free(ppinfo);
            dir = NULL;
            ppinfo = NULL;
            return 0;
        }
    }
}

// Directory iteration functions
fdDir * fs_opendir(const char *pathname){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));

    if(parsePath(pathname, ppinfo) != 0){
        free(ppinfo);
        return NULL;
    }

    DirectoryEntry* targetDir;

    if(ppinfo->index == -2){
        targetDir = rootDirectory;
    }
    else if(ppinfo->index == -1){
        free(ppinfo);
        return NULL;
    } else {
        targetDir = &(ppinfo->parent[ppinfo->index]);
        if (!isDEaDir(targetDir)) {
            free(ppinfo);
            return NULL;
        }
    }

    DirectoryEntry* dirEntries = loadDir(targetDir);
        if (dirEntries == NULL) {
            free(ppinfo);
            return NULL;
        }

    fdDir* dir = malloc(sizeof(fdDir));
        if (dir == NULL) {
            free(dirEntries);
            free(ppinfo);
            return NULL;
        }

    // Initialize the directory structure
    dir->directory = dirEntries;
    dir->dirEntryPosition = 0;
    dir->d_reclen = sizeof(fdDir);
    dir->di = malloc(sizeof(struct fs_diriteminfo));
    if (dir->di == NULL) {
        free(dir);
        free(dirEntries);
        free(ppinfo);
        return NULL;
    }
    
    free(ppinfo);
    ppinfo = NULL;
    return dir;
}

// TO DO:
struct fs_diriteminfo *fs_readdir(fdDir *dirp){
    if (dirp == NULL || dirp->directory == NULL || dirp->di == NULL) {
        return NULL;
    }

    DirectoryEntry* dirEntries = dirp->directory;
    int position = dirp->dirEntryPosition;

    // Search for a valid entry
    while(position < MAX_ENTRIES){
        if (dirEntries[position].inUse){
            // Exit once we find the valid entry
            break;
        }
        position++;
    }

    // Check if we reached the end
    if (position >= MAX_ENTRIES) {
        return NULL;  // No more entries
    }

    dirp->di->d_reclen = sizeof(struct fs_diriteminfo);

    // Copy the filename 
    strncpy(dirp->di->d_name, dirEntries[position].fileName, 255);
    dirp->di->d_name[255] = '\0';

    // Set the file type
    if (dirEntries[position].isDir) {
        dirp->di->fileType = FT_DIRECTORY;
    } else {
        dirp->di->fileType = FT_REGFILE;
    }
    
    // Update the position counter for the next call
    dirp->dirEntryPosition = position + 1;
    
    return dirp->di;
}

int fs_closedir(fdDir *dirp){
    // check to see if we are in a proper directory 
    if (dirp == NULL){
        return -1; 
    }

    // free the directory item info struct
    if (dirp -> di != NULL){
        free(dirp -> di);
        dirp -> di = NULL; 
    }

    // free the dreictory entry array
    if (dirp -> directory != NULL){
        free(dirp -> directory);
        dirp -> directory = NULL; 
    }

    // free the main fdDir struc
    free (dirp);
    dirp = NULL;

    // muy importante to return to caller upon success 
    return 0; 
}


// Misc directory functions
char * fs_getcwd(char *pathname, size_t size) {
    if (pathname == NULL || size == 0) {
        return NULL;
    }
    
    // Get directory name
    strncpy(pathname, currentWorkingPath, size);
    
    // Ensure null-termination
    pathname[size-1] = '\0';
    
    return pathname;
}

//linux chdir
int fs_setcwd(char *pathname){
    if (pathname == NULL) {
        return -1;
    }
    
    ppinfo* ppinfo = malloc(sizeof(ppinfo));
    if (ppinfo == NULL) {
        return -1;
    }

    char *pathCopy = strdup(pathname);
    if (!pathCopy) {
        free(ppinfo);
        return -1;
    }

    if (parsePath(pathCopy, ppinfo) == -1) {
        free(ppinfo); 
        free(pathCopy);
        return -1; 
    }

    // Special case for the root directory 
    if (ppinfo-> index == -2){
        currentWorkingDirectory = rootDirectory;
        strncpy(currentWorkingPath, "/", MAXDIR_LEN);
        free(ppinfo);
        ppinfo = NULL;
        return 0;
    }

  
   DirectoryEntry *target = &(ppinfo->parent[ppinfo->index]);

    // Make sure it's a directory
    if (!target->isDir) {
        free(ppinfo);
        free(pathCopy);
        return -1;
    }

    // Allocate space for new directory block
    int blocksToRead = (target->fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    DirectoryEntry *newDir = malloc(sizeof(DirectoryEntry) * MAX_ENTRIES);
    if (newDir == NULL) {
        free(ppinfo);
        free(pathCopy);
        return -1;
    }

    // Read directory block into memory
    LBAread(newDir, blocksToRead, target->startBlock);

    // Set CWD to new block
    currentWorkingDirectory = newDir;

    if (pathname[0] == '/') {
    // Absolute path — copy directly
        strncpy(currentWorkingPath, pathname, MAXDIR_LEN);
        currentWorkingPath[MAXDIR_LEN - 1] = '\0';
    } else {
        // Relative path — append to existing path
        if (strcmp(currentWorkingPath, "/") == 0) {
            snprintf(currentWorkingPath, MAXDIR_LEN, "/%s", pathname);
        } else {
            strncat(currentWorkingPath, "/", MAXDIR_LEN - strlen(currentWorkingPath) - 1);
            strncat(currentWorkingPath, pathname, MAXDIR_LEN- strlen(currentWorkingPath) - 1);
        }
    }
    currentWorkingPath[MAXDIR_LEN - 1] = '\0';

    free(ppinfo);
    ppinfo = NULL;

    return 0;
} 

int fs_isFile(char * filename){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));
    char *savePtr;
    char *fileStr;

    // If filename includes a ".", then there is no need to do anything else
    fileStr = strtok_r(filename, ".", &savePtr); 
    if(fileStr != NULL){
        return 1;
    }else{
        if(parsePath(filename, ppinfo) == -1){
            // Path does not exist
            return 0;
        }
        int indx = ppinfo->index;
        DirectoryEntry  *dir = &(ppinfo->parent[indx]);
        // Checks isDir, which holds type for directory entries
        if(isDEaDir(dir) != 1){
            // File is actually a file
            free(ppinfo);
            free(dir);
            ppinfo = NULL;
            dir = NULL;
            return 1;
        }else{
            // File is a directory
            free(ppinfo);
            free(dir);
            ppinfo = NULL;
            dir = NULL;
            return 0;
        }
    }
}

int fs_isDir(char * pathname){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));

    if(parsePath(pathname, ppinfo) == -1){
        // Path does not exist
        return 0;
    }else{
        int indx = ppinfo->index;
        // Checks isDir, which holds type for directory entries
        DirectoryEntry  *dir = &(ppinfo->parent[indx]);
        if(isDEaDir(dir) == 1){
            // Path is a directory
            free(ppinfo);
            ppinfo = NULL;
            return 1;
        }else{
            // Path is a file
            free(ppinfo);
            ppinfo = NULL;
            return 0;
        }
    }
}

int fs_delete(char* filename){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));
    int indx;
    int totalBlocks;
    int firstBlock;

    if(parsePath(filename, ppinfo) == -1){
        // File does no exist
        return -1;
    }else{
        if(fs_isFile(filename) == 1){
            // Deletes file by freeing the memory associated with it
            int indx = ppinfo->index;
            firstBlock = ppinfo->parent[indx].startBlock;
            totalBlocks = (ppinfo->parent[indx].fileSize + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
            releaseBlocks(firstBlock, totalBlocks);
            strcpy(ppinfo->parent[indx].fileName, "NULL");
            ppinfo->parent[indx].permissions = 0;
            ppinfo->parent[indx].fileSize = 0;
            ppinfo->parent[indx].startBlock = 0;
            ppinfo->parent[indx].createdTime = 0;
            ppinfo->parent[indx].lastModified = 0;
            ppinfo->parent[indx].lastAccessed = 0;
            ppinfo->parent[indx].isDir = NULL;
            ppinfo->parent[indx].inUse = false;
            free(ppinfo);
            ppinfo = NULL;
            return 0;
        }else{
            return -1;
        }
    }
}

int parsePath(char* pathname, ppinfo* ppi){
    DirectoryEntry* parent;
    DirectoryEntry* startParent;
    char* savePtr;
    char* token1;
    char* token2;

    if(pathname == NULL) return (-1);
    if(pathname[0] == '/') {
        startParent = rootDirectory;
    }else{
        startParent = currentWorkingDirectory;
    }
    parent = startParent;

    token1 = strtok_r(pathname, "/", &savePtr);

    if(token1 == NULL) {
        if(pathname[0] == '/'){
            ppi->parent = parent;
            ppi->index = -2;
            ppi->lastElementName = NULL;
            return 0;
        }else{
            return -1;
        }
    }

    while(1){
        int idx = findInDirectory(parent, token1);
        token2 = strtok_r(NULL, "/", &savePtr);

        if(token2 == NULL){
            ppi->parent = parent;
            ppi->index = idx;
            ppi->lastElementName = token1;
            return 0;
        }else{
            if(idx == -1){
                return -2;
            }
            if(!isDEaDir(&(parent[idx]))) return (-1);

        DirectoryEntry* tempParent = loadDir(&(parent[idx]));

        if(parent != startParent){
            free(parent);
        }

        parent = tempParent;
        token1 = token2;

        }
    }
}

int findInDirectory(DirectoryEntry* parent, char* token){

    // Iterate through the every possible entry 
    for(int i = 0; i < MAX_ENTRIES; i++ ){
        if(strcmp(parent[i].fileName, token) == 0){
            return i;
        }
    }

    // Directory entry does not exist;
    return -1;
}

DirectoryEntry* loadDir(DirectoryEntry* targetDir){
    // Calculate how many entries we have
    int numEntries = targetDir->fileSize / sizeof(DirectoryEntry);
    // Calculate how many blocks are needed to store the directory
    int blocksNeeded = (targetDir->fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    DirectoryEntry* dirBuffer = (DirectoryEntry*)malloc(targetDir->fileSize);
    if (dirBuffer == NULL) {
        return NULL; 
    }
    // Read the directory blocks from disk
    int startBlock = targetDir->startBlock;
    // LBAread returns the number of blocks actually read
    if (LBAread(dirBuffer, blocksNeeded, startBlock) != blocksNeeded) {
        free(dirBuffer);
        return NULL;
    }
    // Update access time for the directory
    targetDir->lastAccessed = time(NULL);
    return dirBuffer;
}

// Returns 1 if DE is a directory, 0 if false
int isDEaDir(DirectoryEntry* targetDir){
    if(targetDir->isDir == false){
        return 0;
    }
    else return 1; 
}

int fs_stat(const char *path, struct fs_stat *buf){
    printf("\n Printing metadata \n");

    if (path == NULL || buf == NULL){
		return -1; 
	}

    // need to allocate memeory to parse the directory path we want
	ppinfo* info = malloc(sizeof(ppinfo));

	// parse the path to locate desire file in directory struct
    int result = parsePath((char *)path, info);
    if(result != 0){
		free(info);
		return -1; 
    }  
    
    // invalid path; index of -1 --> actual file does not exist 
    if(info->index == -1){
        free(info);
        return -1; 
    }

    DirectoryEntry entry = info->parent[info->index];

    // populate fs_stat struct
    buf->st_size = entry.fileSize;
    buf->st_blksize = BLOCK_SIZE;  
    buf->st_blocks = (entry.fileSize + 511) / 512; 
    buf->st_accesstime = entry.lastAccessed;
    buf->st_modtime = entry.lastModified;
    buf->st_createtime = entry.createdTime;

    free(info);

    return 0; 
}

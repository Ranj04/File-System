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
#include <freeSpace.h>

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

    if(parsePath(pathname, ppinfo) != 0){
        free(ppinfo);
        return -1;
    }else{
        int indx = ppinfo->index;
        DirectoryEntry  *dir = &(ppinfo->parent[indx]);
        if(!isDEaDir(dir) || ppinfo->lastElementName != NULL){
            free(ppinfo);
            return -1;
        }else{
            ppinfo->parent[indx].inUse = false;
            free(ppinfo);
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
    
    if (currentWorkingDirectory == rootDirectory) {
        strncpy(pathname, "/", size);
        return pathname;
    }
    
    // Get directory name
    strncpy(pathname, currentWorkingDirectory->fileName, size);
    
    // Ensure null-termination
    pathname[size-1] = '\0';
    
    return pathname;
}

//linux chdir
int fs_setcwd(char *pathname){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));

    if (parsePath(pathname, ppinfo) == -1) {
        return -1; 
    }

    // Special case for the root directory 
    if (ppinfo-> index == -2){
        currentWorkingDirectory = rootDirectory;

        free(ppinfo);
        ppinfo == NULL;
        return 0;
    }

    if(fs_isDir(pathname) != 1){
        free(ppinfo);
        return -1; // Not a directory 
    }

    // Cleaning up path
    char *savePtr;
    char *newPath = strtok_r(pathname, "/", &savePtr);
    char *reducedPath = malloc(100);
    int i = 0;
    while (newPath != NULL){
        if(strstr(newPath, "..") != NULL){
            i = 0;
        }else if(strstr(newPath, ".") == NULL){
            reducedPath[i] = newPath;
            i++;
        }
    }
    char *finalPath[i];
    for(int j = 0; j < i; j++){
        finalPath[j] = reducedPath[j];
        printf("/%s", finalPath[j]);
    }

    // Set CWD to the target directory
    currentWorkingDirectory = &(ppinfo->parent[ppinfo->index]); 

    free(ppinfo);
    ppinfo = NULL;

    return 0;
} 

int fs_isFile(char * filename){
    int isDirectory = 0;
    char* fileStr;
    char* savePtr;

    // If filename includes a ".", then there is no need to do anything else
    fileStr = strtok_r(filename, ".", &savePtr); 
    if(fileStr != NULL){
        return 1;
    }else{
        // isDir functions the same as isFile, there is no need to write duplicate code
        isDirectory = fs_isDir(filename);
        // Directory entries can only be either a file or directory
        if(isDirectory == 1){
            // File is a directory
            return 0;
        }else{
            // File is actually a file
            return 1;
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

    if(parsePath(filename, ppinfo) == -1){
        // File does no exist
        return -1;
    }else{
        if(fs_isFile(filename) == 1){
            // Deletes file by declaring space available
            int indx = ppinfo->index;
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

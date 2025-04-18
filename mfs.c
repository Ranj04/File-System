/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Ty Bohlander
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

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode){
    ppinfo* ppinfo = malloc(sizeof(ppinfo));

    if(parsePath(pathname, ppinfo) != 0){
        free(ppinfo);
        return NULL;
    }

    //Check if a file/directory with the same name already exists in that location
    if(ppinfo->index != -1){
        free(ppinfo);
        return -1;
    }

    uint64_t startBlock = createDirectory(ppinfo->parent, MAX_ENTRIES);

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

int fs_rmdir(const char *pathname);

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
    ppinfo == NULL;
    return dir;
}

// TO DO:
struct fs_diriteminfo *fs_readdir(fdDir *dirp){

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
    strncpy(pathname, currentWorkingDirectory->fileName, size);
    return(pathname); 
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

    /**** Handle cleaning up path name here ****/

    // Set CWD to the target directory
    currentWorkingDirectory = &(ppinfo->parent[ppinfo->index]); 

    free(ppinfo);
    ppinfo = NULL;

    return 0;
} 

// TO DO 
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

int parsePath(char* path, ppinfo* ppi){
    DirectoryEntry* parent; 
    DirectoryEntry* startParent; 
    char* savePtr; 
    char* token1, token2;

    if (path == NULL){
        return -1; 
    }

    if (path[0] == '/'){
        startParent = rootDirectory;
    }
    else {
        startParent = currentWorkingDirectory;
    }
    parent = startParent;

    token1 = strtok_r(path, "/", &savePtr);
    if(token1 == NULL){
        if(path[0] == '/'){
            ppi->parent = parent;
            ppi->index = -2; 
            ppi->lastElementName = NULL;
            return 0; 
        }
        else {
            return -1; 
        }
    }
    int idx = findInDirectory(parent, token1);
    token2 = strtok_r(NULL, "/", savePtr);

    if(token2 == NULL){
        ppi->parent = parent;
        ppi->index = idx;
        ppi->lastElementName = token1; 
        return 0; 
    }
    else {
        if (idx == -1){
            return -2; 
        }
        if(!isDEaDir(&parent[idx])){
            return -1; 
        }
        DirectoryEntry* tempParent = loadDir(&parent[idx]);
        if(parent != startParent){
            free(parent);
        }
        
        parent = tempParent;
        token1 = token2;
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

// TO DO: 
DirectoryEntry* loadDir(DirectoryEntry* targetDir){

}

// Returns 1 if DE is a directory, 0 if false
int isDEaDir(DirectoryEntry* targetDir){
    if(targetDir->isDir == false){
        return 0;
    }
    else return 1; 
}
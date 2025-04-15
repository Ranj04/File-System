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
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

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
        }else {
            parent = tempParent;
            token1 = token2;
        }
        return 0;
    }
}

int findInDirectory(DirectoryEntry* parent, char* token){

    // NEED TO CHANGE, will not work if file sizes are different 
    int numEntries = parent[0].fileSize / sizeof(DirectoryEntry);

    for(int i = 0; i < numEntries; i++ ){
        if(strcmp(parent[i].fileName, token) == 0){
            return i;
        }
    }

    // Directory entry does not exist;
    return -1;
}/**************************************************************
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
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

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
        }else {
            parent = tempParent;
            token1 = token2;
        }
        return 0;
    }
}

int findInDirectory(DirectoryEntry* parent, char* token){

    // NEED TO CHANGE, will not work if file sizes are different 
    int numEntries = parent[0].fileSize / sizeof(DirectoryEntry);

    for(int i = 0; i < numEntries; i++ ){
        if(strcmp(parent[i].fileName, token) == 0){
            return i;
        }
    }

    // Directory entry does not exist;
    return -1;
}

DirectoryEntry* loadDir(DirectoryEntry* targetDir){

}

// Returns 1 if DE is a directory, 0 if false
int isDEaDir(DirectoryEntry* targetDir){
    if(targetDir->isDir == false){
        return 0;
    }
    else return 1; 
}

DirectoryEntry* loadDir(DirectoryEntry* targetDir){

}

// Returns 1 if DE is a directory, 0 if false
int isDEaDir(DirectoryEntry* targetDir){
    if(targetDir->isDir == false){
        return 0;
    }
    else return 1; 
}
# Mini Filesystem (MFS) in C

A tiny teaching filesystem with block I/O plus a demo shell (`fsshell`) that exercises common file and directory operations.

# Author: Ranjiv Jithendran
    Email: rjithendran@sfsu.edu / ranjiv.jithendran@gmail.com

- **Block size:** 512 bytes  
- **Typical volume size (for demos/tests):** 500,000 – 10,000,000 bytes  
- Public API is declared in `mfs.h` (you may adapt it for your design, especially `fdDir`).

---

## ✨ Features

- **Block I/O** via `LBAread` / `LBAwrite`
- Basic **file** and **directory** operations
- Interactive shell (`fsshell`) with commands like `ls`, `cp`, `mv`, `md`, `rm`, `touch`, `cat`, `cd`, `pwd`, `cp2l`, `cp2fs`, `history`, `help`
- Pluggable on-disk layout (implement your own metadata structures)

---

## 🧱 Block I/O API

```c
#include <stdint.h>

uint64_t LBAwrite(void *buffer, uint64_t lbaCount, uint64_t lbaPosition);
uint64_t LBAread (void *buffer, uint64_t lbaCount, uint64_t lbaPosition);

```
## Filesystem API (dirs, paths, metadata):
int     fs_mkdir   (const char *pathname, mode_t mode);
int     fs_rmdir   (const char *pathname);
fdDir * fs_opendir (const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int     fs_closedir(fdDir *dirp);

char  * fs_getcwd  (char *pathbuffer, size_t size);
int     fs_setcwd  (char *pathname);      // like POSIX chdir
int     fs_isFile  (char *filename);      // 1 if file, 0 otherwise
int     fs_isDir   (char *pathname);      // 1 if directory, 0 otherwise
int     fs_delete  (char *filename);      // remove a file

## Directory entry returned by fs_readdir (not your on-disk format):
struct fs_diriteminfo {
    unsigned short d_reclen;   /* length of this record */
    unsigned char  fileType;   /* e.g., file/dir */
    char           d_name[256];/* up to 255 chars + NUL */
};

## Stat-like metadata:
int fs_stat(const char *filename, struct fs_stat *buf);

struct fs_stat {
    off_t     st_size;       /* total size, in bytes */
    blksize_t st_blksize;    /* preferred I/O block size */
    blkcnt_t  st_blocks;     /* number of 512B blocks allocated */
    time_t    st_accesstime; /* last access time */
    time_t    st_modtime;    /* last modification time */
    time_t    st_createtime; /* creation time */
    /* add any extra attributes your design needs */
};

## Build & Run:
Prerequisites

Linux/macOS: gcc and make

Windows: MSYS2/MinGW-w64 (gcc, mingw32-make) or WSL

Build (with Makefile):

make                      # produces fsshell (or fsshell.exe on Windows)


Direct build (tiny demo):
gcc -std=c99 main.c <your_sources>.c -o fsshell


Initialize volume & run: 
startPartitionSystem(volumeSizeBytes, 512);

Example shell usage:
./fsshell
> md docs
> touch docs/readme.txt
> cp2fs host.txt docs/readme.txt
> ls
> cat docs/readme.txt
> pwd
> help

## Shell commands in fsshell:
| Command   | Description              | Example                     |
| --------- | ------------------------ | --------------------------- |
| `ls`      | List directory contents  | `ls` / `ls path`            |
| `cp`      | Copy within this FS      | `cp src [dest]`             |
| `mv`      | Move/rename              | `mv src dest`               |
| `md`      | Make directory           | `md projects`               |
| `rm`      | Remove file/dir          | `rm file.txt` / `rm -r dir` |
| `touch`   | Create empty file        | `touch notes.txt`           |
| `cat`     | Print file (limited)     | `cat notes.txt`             |
| `cp2l`    | Copy from this FS → host | `cp2l fs/file host/file`    |
| `cp2fs`   | Copy from host → this FS | `cp2fs host/file fs/file`   |
| `cd`      | Change directory         | `cd docs`                   |
| `pwd`     | Print working directory  | `pwd`                       |
| `history` | Show command history     | `history`                   |
| `help`    | Show help                | `help`                      |


